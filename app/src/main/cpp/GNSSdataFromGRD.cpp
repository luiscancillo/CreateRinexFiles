/** @file GNSSdataFromGRD.cpp
 * Contains the implementation of the GNSSdataFromGRD class for ORD and NRD raw data files.
 *
 */
#include "GNSSdataFromGRD.h"

#define LOG_MSG_COUNT (" @" + to_string(msgCount))

/**Constructs a GNSSdataFromGRD object using parameter passed.
 *
 *@param pl a pointer to the Logger to be used to record logging messages
 */
GNSSdataFromGRD::GNSSdataFromGRD(Logger* pl) {
    plog = pl;
    dynamicLog = false;
    setInitValues();
}

/**Constructs a GNSSdataFromGRD object logging data into the stderr.
 *
 */
GNSSdataFromGRD::GNSSdataFromGRD() {
    plog = new Logger();
    dynamicLog = true;
    setInitValues();
}

/**Destroys a GNSSdataFromGRD object
 */
GNSSdataFromGRD::~GNSSdataFromGRD(void) {
    if (dynamicLog) delete plog;
}

/**openInputGRD opens the GRD input file with the name and in the path given.
 * @param inputFilePath the full path to the file to be open
 * @param inputFileName the name of the file to be open
 * @return true if input file succesfully open, false otherwhise
 */
bool GNSSdataFromGRD::openInputGRD(string inputFilePath, string inputFileName) {
    //open input raw data file
    bool retVal = true;
    string inFileName = inputFilePath + inputFileName;
    if ((grdFile = fopen(inFileName.c_str(), "r")) == NULL) {
        plog->warning(LOG_MSG_ERROPEN + inFileName);
        return false;
    }
    return true;
}

/**rewindInputGRD rewinds the GRD input file already open
 */
void GNSSdataFromGRD::rewindInputGRD() {
    rewind(grdFile);
}

/**closeInputGRD closes the currently open input GRD file
 */
void GNSSdataFromGRD::closeInputGRD() {
    fclose(grdFile);
}

/**collectHeaderData extracts data from the ORD or NRD file for RINEX file header for further printing.
 * Also other parameters useful for processing observation or navigation data can be collected here.
 * <p>To collect this data the whole file is parsed from begin to end. Message types not containing
 * data above mentioned are skipped.
 *
 * @param rinex the RinexData object where header data will be saved
 * @return true if all above described header data are properly extracted, false otherwise
 */
bool GNSSdataFromGRD::collectHeaderData(RinexData &rinex) {
    int msgType;
    char msgBuffer[100];
    double dvoid;
    int ivoid;
    string svoid;
    string msgError;
    string pgm, runby, date;
    string observer, agency;
    string rcvNum, rcvType, rcvVer;
    //message parameters given by Android
    char constId;       //the constellation identifier this satellite belongs
    int satNum, satIdx;		//the satellite number this navigation message belongs and its index
    int strNum;     //GLONASS navigation message string number
    int frame;      //GLONASS navigation message frame number
    //for data extracted from the navigation message
    unsigned int gloStrWord[GLO_STRWORDS]; //a place to store the bits stream of a GLONASS message
    int nA;     //the slot number got from almanac data
    int hnA;
    char sgnl[4] = {0};
    bool tofoUnset = true;   //time of first observation not set
    rewind(grdFile);
    msgCount = 0;
    while ((feof(grdFile) == 0) && (fscanf(grdFile, "%d;", &msgType) == 1)) { 	//there are messages in the raw data file
        msgCount++;
        switch(msgType) {
            case MT_GRDVER:
            case MT_RINEXVER:
            case MT_PGM:
            case MT_RUN_BY:
            case MT_DATE:
            case MT_INTERVALMS:
            case MT_MARKER_NAME:
            case MT_MARKER_TYPE:
            case MT_OBSERVER:
            case MT_AGENCY:
            case MT_RECNUM:
            case MT_DVTYPE:
            case MT_DVVER:
            case MT_LLA:
            case MT_FIT:
                if (fgets(msgBuffer, sizeof msgBuffer, grdFile) == msgBuffer) {
                    trimBuffer(msgBuffer, "\r \t\f\v\n");
                    processHdData(rinex, msgType, string(msgBuffer));
                } else plog->warning(LOG_MSG_ERRO + getMsgDescription(msgType) + " params" + LOG_MSG_COUNT);
                continue;   //fgets already reads the EOL (skipToEOM not neccesary)!
            case MT_SATOBS:
                if (fscanf(grdFile, "%c%d;%c%c", &constId, &satNum, sgnl, sgnl+1) == 4) {
                    if (addSignal(constId, string(sgnl)))
                        plog->fine(getMsgDescription(msgType) + ": added " + string(1, constId) +
                                            MSG_SPACE + string(sgnl) + LOG_MSG_COUNT);
                } else plog->warning(LOG_MSG_ERRO + getMsgDescription(msgType) + " params" + LOG_MSG_COUNT);
                break;
            case MT_EPOCH:
                if (tofoUnset) {
                    getAndSetEpochTime(rinex, dvoid, ivoid, "Fist epoch");
                    rinex.setHdLnData(rinex.TOFO);
                    tofoUnset = false;
                }
                break;
            case MT_SATNAV_GLONASS_L1_CA:
                if (!readGLOEpochNav(constId, satNum, strNum, frame, gloStrWord)) return false;
                //get the satellite index from its number
                if ((satIdx = gloSatIdx(satNum)) >= GLO_MAXSATELLITES) {
                    plog->warning("Bad GLO satNum " + to_string(satNum) + " in strNum " + to_string(strNum) + LOG_MSG_COUNT);
                    break;
                }
                switch (strNum) {
                    case 6:
                    case 8:
                    case 10:
                    case 12:
                    case 14:
                        //get from almanac data the slot number (nA) in bits 77-73 (see GLONASS ICD for details)
                        //and prepare slot - carrier frequency table to receive the value corresponding to this slot (if not already received)
                        nA = getBits(gloStrWord, 72, 5);
                        if (nA >= GLO_MINOSN && nA <= GLO_MAXOSN) {
                            nAhnA[satIdx].nA = nA;
                            nAhnA[satIdx].strFhnA = strNum + 1;	//set the string number where continuation data should come
                        } else plog->warning("Bad GLO slot " + to_string(nA) + " in strNum " + to_string(strNum) + LOG_MSG_COUNT);
                        break;
                    case 7:
                    case 9:
                    case 11:
                    case 13:
                    case 15:
                        //check in the slot - carrier frequency table the expected string number for this satellite index
                        //if current string is the expected one to provide the carrier frequency data, store it
                        if (nAhnA[satIdx].strFhnA == strNum) {
                            int inx = nAhnA[satIdx].nA - 1;
                            if ((inx >= 0) && (inx <= GLO_MAXSATELLITES)) {
                                hnA = getBits(gloStrWord, 9, 5);	//HnA in almanac: bits 14-10
                                if (hnA >= 25) hnA -= 32;	//set negative values as per table 4.11 of the GLONASS ICD
                                carrierFreq[inx] = hnA;
                            } else plog->warning("Bad GLO slot " + to_string(inx) + " slot-carrier frequency table " + to_string(satIdx) + LOG_MSG_COUNT);
                        }
                        break;
                    default:
                        break;
                }
                break;
            case MT_SATNAV_GPS_l1_CA:
            case MT_MT_SATNAV_GALILEO_FNAV:
            case MT_SATNAV_BEIDOU_D1:
                break;
            default:
                plog->warning(LOG_MSG_ERRO + getMsgDescription(msgType) + LOG_MSG_COUNT);
                break;
        }
        skipToEOM();
    }
    setHdSys(rinex);
    return true;
}

/**collectEpochObsData extracts observation and time data from observation raw data (ORD) file messages for 
 * an epoch.
 *<p>Epoch RINEX data are contained in a sequence of MT_EPOCH {MT_SATOBS} messages.
 *<p>The epoch starts with a MT_EPOCH message. It contains clock data for the current epoch, including its time,
 * and the number of MT_SATOBS that would follow.
 *<p>Each MT_SATOBS message contains observables for a given satellite (in each epoch the receiver sends  a MT_SATOBS
 * message for each satellite being tracked).
 * An unexpected end of epoch occurs when the number of MT_SATOBS messages does not correspond with what has been
 * stated in MT_EPOCH. This event would be logged at warning level.
 *<p>This method acquires data reading messages recorded in the raw data file according to the above sequence and
 * computes the obsevables from the acquired raw data. These data are saved in the RINEX data structure for further
 * generation/printing of RINEX observation records in the observation file.
 * Method returns when it is read the last message of the current epoch.
 *<p>Other messages in the input binary file different from  MT_EPOCH ot MT_SATOBS are ignored.
 *
 * @param rinex the RinexData object where data got from receiver will be placed
 * @param msgCount the input file message counter
 * @return true when observation data from an epoch messages have been acquired, false otherwise (End Of File reached)
 */
bool GNSSdataFromGRD::collectEpochObsData(RinexData &rinex) {
    //variables to get data from ORD observation records
    int msgType;
    char constId, ambiguous;
    int satellite, synchState;
    double carrierPhase, cn0db, timeOffsetNanos, psrangeRate, carrierFrequencyMHz;
    char sgnl[4] = {0};
    //variables to obtain observables
    //This app version assumes for tRxGNSS the GPS time reference (tRxGNSS is here tRxGPS)
    long long tTx = 0;  //the satellite transmitted clock in nanosec. (from receivedSatTimeNanos given by Android I/F)
    long long tRx = 0;  //the receiver clock in nanosec. from the beginning of the current GPS week (is referenced to the GPS constellation time frame)
    double tow = 0;     //time of week in seconds from the beginning of the current GPS week
    double pseudorange = 0.0;
    double dopplerShift = 0.0;
    int lli = 0;    //loss o lock indicator as per RINEX V3.01
    int sn_rnx = 0;   //signal strength as per RINEX V3.01
    int numMeasur = 0; //number of satellite measurements in this epoch
    while (fscanf(grdFile, "%d;", &msgType) == 1) {
        msgCount++;
        switch(msgType) {
            case MT_EPOCH:
                if (numMeasur > 0) plog->warning(LOG_MSG_ERRO + "Few MT_SATOBS in epoch" + LOG_MSG_COUNT);
                tRx = getAndSetEpochTime(rinex, tow, numMeasur, "Epoch");
                break;
            case MT_SATOBS:
                if (numMeasur == 0) plog->warning(LOG_MSG_ERRO + "MT_SATOBS before MT_EPOCH" + LOG_MSG_COUNT);
                else {
                    if (fscanf(grdFile, "%c%d;%c%c;%c;%d;%lld;%lf;%lf;%lf;%lf;%lf",
                               &constId, &satellite, sgnl+1, sgnl+2, &ambiguous,
                               &synchState, &tTx, &carrierPhase, &cn0db,
                               &timeOffsetNanos, &carrierFrequencyMHz, &psrangeRate) == 12) {
                        //from data available compute signal to noise RINEX index, paseudorange and doppler shift
                        sn_rnx = (int) (cn0db / 6);
                        if (sn_rnx < 1) sn_rnx = 1;
                        else if (sn_rnx > 9) sn_rnx = 1;
                        //TBD LLI indicator: bits 0, 1, or 2 set?
                        sgnl[0] = 'C';
                        pseudorange = ((double)(tRx - tTx) - timeOffsetNanos) * SPEED_OF_LIGTH_MxNS;
                        rinex.saveObsData(constId, satellite, string(sgnl), pseudorange, lli, sn_rnx, tow);
                        sgnl[0] = 'L';
                        rinex.saveObsData(constId, satellite, string(sgnl), carrierPhase, lli, sn_rnx, tow);
                        sgnl[0] = 'D';
                        dopplerShift = psrangeRate * carrierFrequencyMHz * DOPPLER_FACTOR;
                        rinex.saveObsData(constId, satellite, string(sgnl), dopplerShift, lli, sn_rnx, tow);
                        sgnl[0] = 'S';
                        rinex.saveObsData(constId, satellite, string(sgnl), cn0db, lli, sn_rnx, tow);
                        plog->fine(getMsgDescription(msgType) + MSG_SPACE + string(1, constId) + to_string(satellite) + MSG_SPACE +
                                     to_string(pseudorange) + MSG_SPACE + to_string(carrierPhase) + MSG_SPACE +
                                     to_string(dopplerShift) + MSG_SPACE + to_string(cn0db) + LOG_MSG_COUNT);
                        if (--numMeasur == 0) {
                            skipToEOM();
                            return true;
                        }
                    } else plog->warning(LOG_MSG_ERRO + "MT_SATOBS params" + LOG_MSG_COUNT);
                }
                break;
            case MT_SATNAV_GPS_l1_CA:
            case MT_SATNAV_GLONASS_L1_CA:
            case MT_MT_SATNAV_GALILEO_FNAV:
            case MT_SATNAV_BEIDOU_D1:
                plog->warning(LOG_MSG_ERRO + LOG_MSG_NINO + LOG_MSG_COUNT);
            default:
                break;
        }
        skipToEOM();
    }
    return false;
}

/**collectNavData iterates over the input raw data file extracting navigation mesages to process their data.
 * Data extracted are saved in a RinexData object for futher printing in a navigation RINEX file.
 * 
 * @param rinex the RinexData object where data got from receiver will be placed
 * @return true when navigation data from an epoch messages have been acquired, false otherwise (End Of File reached)
 */
bool GNSSdataFromGRD::collectNavData(RinexData &rinex) {
    int msgType;
    bool bvoid;

    while (fscanf(grdFile, "%d;", &msgType) == 1) {
        switch(msgType) {
            case MT_SATNAV_GPS_l1_CA:
            case MT_SATNAV_BEIDOU_D1:
                bvoid = collectGPSEpochNav(rinex);
                break;
            case MT_SATNAV_GLONASS_L1_CA:
                bvoid = collectGLOEpochNav(rinex);
                break;
            case MT_MT_SATNAV_GALILEO_FNAV:
                break;
            case MT_EPOCH:
            case MT_SATOBS:
                plog->severe(LOG_MSG_ERRO + LOG_MSG_NONI);
            default:
                break;
        }
        skipToEOM();
    }
    return false;
}

/**processHdData sets the Rinex header record data from the contens of the given message.
 * Each message type containts data to be processed and stored in the corresponding header record.
 *
 * @param rinex the RinexData object which header data will be updated
 * @param msgType the raw data file message type
 * @param msgContent the content of the message with data to be extracted
 */
void GNSSdataFromGRD::processHdData(RinexData &rinex, int msgType, string msgContent) {
    double da, db, dc;
    double x, y, z;
    string msgError;
    vector<string> selElements;
    int logLevel = 0;
    double dvoid = 0.0;
    string svoid = string();
    plog->fine(getMsgDescription(msgType) + ":" + msgContent + LOG_MSG_COUNT);
    try {
        switch(msgType) {
            case MT_GRDVER:
                plog->warning("MT_GRDVER currently not processed");
                break;
            case MT_PGM:
                rinex.setHdLnData(rinex.RUNBY, msgContent, svoid, svoid);
                break;
            case MT_DVTYPE:
                rinex.setHdLnData(rinex.RECEIVER, svoid, msgContent, svoid);
                break;
            case MT_DVVER:
                rinex.setHdLnData(rinex.RECEIVER, svoid, svoid, msgContent);
                break;
            case MT_LLA:
                if (sscanf(msgContent.c_str(), "%lf;%lf;%lf", &da, &db, &dc) == 3) {
                    llaTOxyz(da * dgrToRads, db * dgrToRads, dc * dgrToRads, x, y, z);
                    rinex.setHdLnData(rinex.APPXYZ, x, y, z);
                } else plog->severe(LOG_MSG_ERRO + getMsgDescription(msgType) + " params" + LOG_MSG_COUNT);
                break;
            case MT_DATE:      //Date of the file
                rinex.setHdLnData(rinex.RUNBY, svoid, svoid, msgContent);
                break;
            case MT_INTERVALMS:
                rinex.setHdLnData(rinex.INT, stod(msgContent) / 1000., dvoid, dvoid);
                break;
            case MT_RINEXVER:
                rinex.setHdLnData(rinex.VERSION, stod(msgContent), dvoid, dvoid);
                break;
            case MT_SITE:
                if (!msgContent.empty()) siteName = msgContent;
                break;
            case MT_RUN_BY:
                rinex.setHdLnData(rinex.RUNBY, svoid, msgContent, svoid);
                break;
            case MT_MARKER_NAME:
                rinex.setHdLnData(rinex.MRKNAME, msgContent, svoid, svoid);
                break;
            case MT_MARKER_TYPE:
                rinex.setHdLnData(rinex.MRKTYPE, msgContent, svoid, svoid);
                break;
            case MT_OBSERVER:
                rinex.setHdLnData(rinex.AGENCY, msgContent, svoid, svoid);
                break;
            case MT_AGENCY:
                rinex.setHdLnData(rinex.AGENCY, svoid, msgContent, svoid);
                break;
            case MT_RECNUM:
                rinex.setHdLnData(rinex.RECEIVER, msgContent, svoid, svoid);
                break;
            case MT_COMMENT:
                rinex.setHdLnData(rinex.COMM, rinex.RUNBY, msgContent);
                break;
            case MT_MARKER_NUM:
                rinex.setHdLnData(rinex.MRKNUMBER, msgContent, svoid, svoid);
                break;
            case MT_CLKOFFS:
                clkoffset = stoi(msgContent);
                rinex.setHdLnData(rinex.CLKOFFS, clkoffset);
                applyBias = clkoffset == 1? true: false;
                plog->finest(getMsgDescription(msgType) + " applyBias:" + (applyBias?string("TRUE"):string("FALSE")));
                break;
            case MT_FIT:
                if (msgContent.find("TRUE") != string::npos) fitInterval = true;
                else fitInterval = false;
                break;
            case MT_LOGLEVEL:
                plog->setLevel(msgContent);
                break;
            case MT_CONSTELLATIONS:
                selElements = getElements(msgContent, "[], ");
                for (vector<string>::iterator it = selElements.begin(); it != selElements.end(); ++it) {
                    if ((*it).compare("GPS") == 0) *it = "G";
                    else if ((*it).compare("GLONASS") == 0) *it = "R";
                    else if ((*it).compare("GALILEO") == 0) *it = "E";
                    else if ((*it).compare("BEIDOU") == 0) *it = "C";
                    else if ((*it).compare("SBAS") == 0) *it = "S";
                    else if ((*it).compare("QZSS") == 0) *it = "J";
                    else {
                        plog->warning(LOG_MSG_UNKSELSYS + (*it) +  LOG_MSG_COUNT);
                        selElements.erase(it);
                    }
                }
                selSatellites.insert(selSatellites.end(), selElements.begin(), selElements.end());
                break;
            case MT_SATELLITES:
                selElements = getElements(msgContent, "[],;.:- ");
                selSatellites.insert(selSatellites.end(), selElements.begin(), selElements.end());
                break;
            case MT_OBSERVABLES:
                selObservables = getElements(msgContent, "[], ");
                break;
            default:
                plog->warning(LOG_MSG_MT_UNK + to_string(msgType) + LOG_MSG_COUNT);
                break;
        }
    } catch (string error) {
        plog->warning(error + LOG_MSG_COUNT);
    }
}

/**processFilterData sets filtering data (constellations, satellites, observations, if any) to be used when printing
 * RINEX files.
 *
 * @param rinex the RinexData object where filtering data will be set
 */
void GNSSdataFromGRD::processFilterData(RinexData &rinex) {
    rinex.setFilter(selSatellites, selObservables);
}
//PRIVATE METHODS
//===============
/**setInitValues set initial values in class variables and tables containig f.e. conversion parameter
 * used to translate scaled normalized GPS message data to values in actual units.
 * <b>Called by the construtors
 */
void GNSSdataFromGRD::setInitValues() {
    msgCount = 0;
    siteName = defaultSiteName;
    clkoffset = 0;
    applyBias = false;
    fitInterval = false;
    //SV clock data
    GPS_SCALEFACTOR[0][0] = pow(2.0, 4.0);		//T0c
    GPS_SCALEFACTOR[0][1] = pow(2.0, -31.0);	//Af0: SV clock bias
    GPS_SCALEFACTOR[0][2] = pow(2.0, -43.0);	//Af1: SV clock drift
    GPS_SCALEFACTOR[0][3] = pow(2.0, -55.0);	//Af2: SV clock drift rate
    //broadcast orbit 1
    GPS_SCALEFACTOR[1][0] = 1.0;				//IODE
    GPS_SCALEFACTOR[1][1] = pow(2.0, -5.0);	//Crs
    GPS_SCALEFACTOR[1][2] = pow(2.0, -43.0) * ThisPI;	//Delta N
    GPS_SCALEFACTOR[1][3] = pow(2.0, -31.0) * ThisPI;	//M0
    //broadcast orbit 2
    GPS_SCALEFACTOR[2][0] = pow(2.0, -29.0);	//Cuc
    GPS_SCALEFACTOR[2][1] = pow(2.0, -33.0);	//e
    GPS_SCALEFACTOR[2][2] = pow(2.0, -29.0);	//Cus
    GPS_SCALEFACTOR[2][3] = pow(2.0, -19.0);	//sqrt(A) ????
    //broadcast orbit 3
    GPS_SCALEFACTOR[3][0] = pow(2.0, 4.0);		//TOE
    GPS_SCALEFACTOR[3][1] = pow(2.0, -29.0);	//Cic
    GPS_SCALEFACTOR[3][2] = pow(2.0, -31.0) * ThisPI;	//Omega0
    GPS_SCALEFACTOR[3][3] = pow(2.0, -29.0);	//Cis
    //broadcast orbit 4
    GPS_SCALEFACTOR[4][0] = pow(2.0, -31.0) * ThisPI;	//i0
    GPS_SCALEFACTOR[4][1] = pow(2.0, -5.0);	//Crc
    GPS_SCALEFACTOR[4][2] = pow(2.0, -31.0) * ThisPI;	//w
    GPS_SCALEFACTOR[4][3] = pow(2.0, -43.0) * ThisPI;	//w dot
    //broadcast orbit 5
    GPS_SCALEFACTOR[5][0] = pow(2.0, -43.0) * ThisPI;	//Idot ??
    GPS_SCALEFACTOR[5][1] = 1.0;				//codes on L2
    GPS_SCALEFACTOR[5][2] = 1.0;				//GPS week + 1024
    GPS_SCALEFACTOR[5][3] = 1.0;				//L2 P data flag
    //broadcast orbit 6
    GPS_SCALEFACTOR[6][0] = 1.0;				//SV accuracy (index)
    GPS_SCALEFACTOR[6][1] = 1.0;				//SV health
    GPS_SCALEFACTOR[6][2] = pow(2.0, -31.0);	//TGD
    GPS_SCALEFACTOR[6][3] = 1.0;				//IODC
    //broadcast orbit 7
    GPS_SCALEFACTOR[7][0] = 0.01;	//Transmission time of message in sec x 100
    GPS_SCALEFACTOR[7][1] = 1.0;	//Fit interval
    GPS_SCALEFACTOR[7][2] = 0.0;	//Spare
    GPS_SCALEFACTOR[7][3] = 0.0;	//Spare
    //GPS_URA table to obtain in meters the value associated to the satellite accuracy index
    //fill GPS_URA table as per GPS ICD 20.3.3.3.1.3
    GPS_URA[0] = 2.0;		//0
    GPS_URA[1] = 2.8;		//1
    GPS_URA[2] = 4.0;		//2
    GPS_URA[3] = 5.7;		//3
    GPS_URA[4] = 8.0;		//4
    GPS_URA[5] = 11.3;		//5
    GPS_URA[6] = pow(2.0, 4.0);		//<=6
    GPS_URA[7] = pow(2.0, 5.0);
    GPS_URA[8] = pow(2.0, 6.0);
    GPS_URA[9] = pow(2.0, 7.0);
    GPS_URA[10] = pow(2.0, 8.0);
    GPS_URA[11] = pow(2.0, 9.0);
    GPS_URA[12] = pow(2.0, 10.0);
    GPS_URA[13] = pow(2.0, 11.0);
    GPS_URA[14] = pow(2.0, 12.0);
    GPS_URA[15] = 6144.00;
    //SV clock data
    GLO_SCALEFACTOR[0][0] = 1;		//T0c
    GLO_SCALEFACTOR[0][1] = pow(2.0, -30.0);		//-TauN
    GLO_SCALEFACTOR[0][2] = pow(2.0, -40.0);		//+GammaN
    GLO_SCALEFACTOR[0][3] = 1;					//Message frame time
    //broadcast orbit 1
    GLO_SCALEFACTOR[1][0] = pow(2.0, -11.0);	//X
    GLO_SCALEFACTOR[1][1] = pow(2.0, -20.0);	//Vel X
    GLO_SCALEFACTOR[1][2] = pow(2.0, -30.0);	//Accel X
    GLO_SCALEFACTOR[1][3] = 1;					//SV Health
    //broadcast orbit 2
    GLO_SCALEFACTOR[2][0] = pow(2.0, -11.0);	//Y
    GLO_SCALEFACTOR[2][1] = pow(2.0, -20.0);	//Vel Y
    GLO_SCALEFACTOR[2][2] = pow(2.0, -30.0);	//Accel Y
    GLO_SCALEFACTOR[2][3] = 1;					//Frequency number
    //broadcast orbit 3
    GLO_SCALEFACTOR[3][0] = pow(2.0, -11.0);	//Z
    GLO_SCALEFACTOR[3][1] = pow(2.0, -20.0);	//Vel Z
    GLO_SCALEFACTOR[3][2] = pow(2.0, -30.0);	//Accel Z
    GLO_SCALEFACTOR[3][3] = 1;					//Age of oper.
    //set tables to 0
    memset(gpsSatFrame, 0, sizeof gpsSatFrame);
    memset(gloSatFrame, 0, sizeof gloSatFrame);
    memset(nAhnA, 0, sizeof nAhnA);
    memset(carrierFreq, 0, sizeof carrierFreq);
}

/**collectGPSEpochNav gets GPS navigation data from a raw message and store them into satellite ephemeris
 * (bradcast orbit data) of the RinexData object.
 * For GPS L1 C/A, Beidou D1 & Beidou D2, each subframe of the navigation message contains 10 30-bit words.
 * Each word (30 bits) should be fit into the last 30 bits in a 4-byte word (skip B31 and B32), with MSB first,
 * for a total of 40 bytes, covering a time period of 6, 6, and 0.6 seconds, respectively.
 * Only have interest subframes: 1,2,3 & page 18 of subframe 4 (pgID = 56 in GPS ICD Table 20-V)
 *
 * @param rinex	the class instance where data are stored
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectGPSEpochNav(RinexData &rinex) {
    int status;     //the status of the navigation message:0=UNKNOWN, 1=PARITY PASSED, 2=REBUILT
    char constId;   //the constellation identifier this satellite belongs
    int satNum;		//the satellite number this navigation message belongssatt
    int sfrmNum;    //navigation message subframe number
    int pageNum;    //navigation message page number
    int msgSize;    //the message size in bytes
    unsigned int msg[GPS_l1_CA_MSGSIZE]; //to store GPS message bytes from receiver
    unsigned int wd[GPS_SUBFRWORDS];	//a place to store the ten words of the GPS message after being refined
    int bom[8][4];		//a RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
    double bo[8][4];	//the RINEX broadcats orbit arrangement for satellite ephemeris
    double tTag;		//the time tag for ephemeris data
    string logmsg;
    try {
        //read MT_SATNAV_GPS_l1_CA message data
        if (fscanf(grdFile, "%d;%c%02d;%d;%d;%d", &status, &constId, &satNum, &sfrmNum, &pageNum, &msgSize) != 5
            || status < 1
            || constId != 'G'
            || satNum < GPS_MINPRN
            || satNum > GPS_MAXPRN
            || msgSize != GPS_l1_CA_MSGSIZE) {
            plog->warning(LOG_MSG_MT_SATNAV_GPS_l1_CA + LOG_MSG_ERRO + LOG_MSG_ONMS + LOG_MSG_COUNT);
            return false;
        }
        if ((sfrmNum < 1 || sfrmNum > 4) || (sfrmNum == 4 && pageNum != 18)) {
            plog->fine(LOG_MSG_MT_SATNAV_GPS_l1_CA + LOG_MSG_NAVIG + LOG_MSG_COUNT);
            return false;
        }
        for (int i = 0; i < GPS_l1_CA_MSGSIZE; ++i) {
            if (fscanf(grdFile,"%X", msg+i) != 1) {
                plog->warning(LOG_MSG_MT_SATNAV_GPS_l1_CA + LOG_MSG_ERRO + LOG_MSG_ONMS + LOG_MSG_COUNT);
                return false;
            }
        }
        logmsg = LOG_MSG_MT_SATNAV_GPS_l1_CA + " sat:" + to_string(satNum) + " subfr:" + to_string(sfrmNum)
                     + "pg:" + to_string(pageNum);
        //pack message bytes into the ten words with navigation data
        for (int i = 0; i < GPS_SUBFRWORDS; ++i) {
            wd[i] = msg[i*4]<<24 | msg[i*4+1]<<16 | msg[i*4+2]<<8 | msg[i*4+4];
        }
        //remove parity from each GPS word getting the useful 24 bits
        //TBC Note that when D30 is set, data bits are complemented (a non documented SiRF OSP feature)
        for (int i=0; i<GPS_SUBFRWORDS; i++)
            if ((wd[i] & 0x40000000) == 0) wd[i] = (wd[i]>>6) & 0xFFFFFF;
            else wd[i] = ~(wd[i]>>6) & 0xFFFFFF;
        sfrmNum--;   //convert subframe number to its index
        satNum--;   //convert satellite number to its index
        for (int i = 0; i < GPS_SUBFRWORDS; ++i) gpsSatFrame[satNum].gpsSatSubframes[sfrmNum].words[i] = wd[i];
        gpsSatFrame[satNum].gpsSatSubframes[sfrmNum].hasData = true;
        gpsSatFrame[satNum].hasData = true;
        //check if all ephemerides have been received, that is:
        //-subframes 1, 2, and 3 have data
        //-and their data belong belond to the same Issue Of Data (IOD)
        bool allRec = gpsSatFrame[satNum].gpsSatSubframes[0].hasData;
        for (int i = 1; i < 3; ++i) allRec = allRec && gpsSatFrame[satNum].gpsSatSubframes[i].hasData;
        //IODC (8LSB in subframe 1) must be equal to IODE in subframe 2 and IODE in subframe 3
        unsigned int iodcLSB = (gpsSatFrame[satNum].gpsSatSubframes[0].words[7]>>16) & 0xFF;
        allRec &= iodcLSB == ((gpsSatFrame[satNum].gpsSatSubframes[1].words[2]>>16) & 0xFF);
        allRec &= iodcLSB == ((gpsSatFrame[satNum].gpsSatSubframes[2].words[9]>>16) & 0xFF);
        if (allRec) {
            //all ephemerides have been already received; extract and store them into the RINEX object
            if (extractGPSEphemeris(satNum, bom)) {
                tTag = scaleGPSEphemeris(bom, bo);
                rinex.saveNavData('G', satNum, bo, tTag);
                logmsg += " and subframe completed";
            }
            //TBW check if iono data (from subframe 4 page 18) exist in gpsSatFrame[satNum].gpsSatSubframes[3] and process them
            //clear satellite frame storage
            for (int i = 0; i < GPS_MAXSUBFRS; ++i) gpsSatFrame[satNum].gpsSatSubframes[i].hasData = false;
            gpsSatFrame[satNum].hasData = false;
        }
        plog->fine(logmsg + LOG_MSG_COUNT);
    } catch (int error) {
        plog->warning(LOG_MSG_MT_SATNAV_GPS_l1_CA + LOG_MSG_ERRO + to_string((long long) error) + LOG_MSG_COUNT);
        return false;
    }
    return true;
}

/**collectGLOEpochNav gets GLONASS navigation data from a raw message and store them into satellite ephemeris
 * (bradcast orbit data) of the RinexData object.
 * <p>For Glonass L1 C/A, each string contains 85 data bits, including the checksum.
 * <p>These bits should be fit into 11 bytes, with MSB first (skip B86-B88), covering a time period of 2 seconds
 * <p>The satellite number given by the satellite can be:
 * <p>- OSN case: it is in the range 1-24 and is the slot number
 * <p>- FCN case: it is in the range 96-106 obtainded from FCN (-7 to +6) + 100. The slot number is in string 4 bits 15-11

 * @param rinex	the class instance where data are stored
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectGLOEpochNav(RinexData &rinex) {
    //message parameters given by Android
    char constId;   //the constellation identifier this satellite belongs
    int satNum, satIdx;		//the satellite number this navigation message belongssatt
    int strNum;     //navigation message string number
    int frmNum;      //navigation message frame number
    //needed for RINEX
    int bom[8][4];			//the RINEX broadcats orbit like arrangement for satellite ephemeris mantissa (as extracted from nav message)
    double bo[8][4];		//the RINEX broadcats orbit arrangement for satellite ephemeris (after applying scale factors)
    //for data extracted from the navigation message
    unsigned int wd[GLO_STRWORDS]; //a place to store the 84 bits of a GLONASS string
    double tTag;    //the time tag for ephemeris data
    int sltnum;     //the GLONASS slot number extracted from navigation message
    //other
    string logmsg;			//a place to build log messages
    try {
        //read MT_SATNAV_GLONASS_L1_CA message data
        if (!readGLOEpochNav(constId, satNum, strNum, frmNum, wd)) return false;
        if (strNum < 1 || strNum > 5) {
            plog->fine(LOG_MSG_MT_SATNAV_GLONASS_L1_CA + LOG_MSG_NAVIG + LOG_MSG_COUNT);
            return false;
        }
        strNum--;		//convert string number to its index
        if ((satIdx = gloSatIdx(satNum)) < GLO_MAXSATELLITES) {
            for (int i = 0; i < GLO_STRWORDS; ++i) gloSatFrame[satIdx].gloSatStrings[strNum].words[i] = wd[i];
            gloSatFrame[satIdx].gloSatStrings[strNum].hasData = true;
            gloSatFrame[satIdx].hasData = true;
            logmsg += " saved";
        } else {
            plog->warning("GLO sat number not OSN or FCN" + LOG_MSG_COUNT);
            return false;
        }
        //check if all ephemerides have been received
        bool allRec = gloSatFrame[satIdx].gloSatStrings[0].hasData;
        for (int i = 1; i < GLO_MAXSTRS; ++i) allRec = allRec && gloSatFrame[satIdx].gloSatStrings[i].hasData;
        if (allRec) {
            //all ephemerides have been already received; extract and store them into the RINEX object
            if (extractGLOEphemeris(satIdx, bom, sltnum)) {
                tTag = scaleGLOEphemeris(bom, bo);
                rinex.saveNavData('R', sltnum, bo, tTag);
                logmsg += " and frame completed";
            }
            //clear satellite frame storage
            for (int i = 0; i < GLO_MAXSTRS; ++i) gloSatFrame[satIdx].gloSatStrings[i].hasData = false;
            gloSatFrame[satIdx].hasData = false;
        }
        plog->fine(logmsg + LOG_MSG_COUNT);
    } catch (int error) {
        plog->severe(LOG_MSG_MT_SATNAV_GLONASS_L1_CA + LOG_MSG_ERRO + to_string((long long) error));
        return false;
    }
    return true;
}

/**
 * readGLOEpochNav performs the common operations to read a GLONASS navigation message.
 *
 * @param constId constellation identification (G, R, E, ...)
 * @param satNum satellite number in the constellation
 * @param strnum string number
 * @param frame frame number
 * @param wd a place to store the bytes of the navigation message
 * @return true if data succesful read, false otherwise
 */
bool GNSSdataFromGRD::readGLOEpochNav(char &constId, int &satNum, int &strnum, int &frame, unsigned int wd[]) {
    int status;     //the status of the navigation message:0=UNKNOWN, 1=PARITY PASSED, 2=REBUILT
    int msgSize;    //the message size in bytes
    unsigned int msg[GLO_STRWORDS * 4];
    try {
        //read MT_SATNAV_GLONASS_L1_CA message data
        if (fscanf(grdFile, "%d;%c%02d;%d;%d;%d", &status, &constId, &satNum, &strnum, &frame, &msgSize) != 5
            || msgSize != GLO_L1_CA_MSGSIZE
            || status < 1) {
            plog->warning(LOG_MSG_MT_SATNAV_GLONASS_L1_CA + LOG_MSG_ERRO + LOG_MSG_ONMS + LOG_MSG_COUNT);
            return false;
        }
        for (int i = 0; i < GLO_L1_CA_MSGSIZE; ++i) {
            if (fscanf(grdFile,"%X", msg+i) != 1) {
                plog->warning(LOG_MSG_MT_SATNAV_GLONASS_L1_CA + LOG_MSG_ERRO + LOG_MSG_ONMS + LOG_MSG_COUNT);
                return false;
            }
        }
        plog->fine(LOG_MSG_MT_SATNAV_GLONASS_L1_CA + " sat:" + to_string(satNum) +
                     " str:" + to_string(strnum) + "frm:" + to_string(frame) + LOG_MSG_COUNT);
        //pack message bytes into words with navigation data
        for (int i = 0; i < GLO_STRWORDS; ++i) {
            wd[i] = msg[i*4]<<24 | msg[i*4+1]<<16 | msg[i*4+2]<<8 | msg[i*4+3];
        }
    } catch (int error) {
        plog->severe(LOG_MSG_MT_SATNAV_GLONASS_L1_CA + LOG_MSG_ERRO + to_string((long long) error));
        return false;
    }
    return true;
}

/**extractGPSEphemeris extract satellite number and ephemeris from the given navigation message transmitted by GPS
 * satellites.
 * <p>The navigation message data of interest here are stored in GPS frame data words (without parity). To process this data they are
 * arranged in an array of 3 x 15 = 45 words (where 16 bits are effective). Ephemeris are extracted from this array.
 * See SiRF ICD (A.5 Message # 15: Ephemeris Data) for details of the arrangement.
 * <p>The ephemeris extracted are store into a RINEX broadcast orbit like arrangement consisting of  8 lines with 3 ephemeris parameters each.
 * <p>This method stores the MANTISSA of each satellite ephemeris into a broadcast orbit array, whithout applying any scale factor.
 *
 * @param satIdx the satellite index in the gpsSatFrame
 * @param bom an array of broadcats orbit data containing the mantissa of each satellite ephemeris
 * @return true if data properly extracted (SV consistency in the channel data, version data consistency and receiver gives confidence on observables), false otherwise
 */
bool GNSSdataFromGRD::extractGPSEphemeris(int satIdx, int (&bom)[8][4]) {
    unsigned int navW[45];	//a place to pack message data as per MID 15 (see SiRF ICD)
    unsigned int sv;
    //pack 3 frames data as per MID 15 (see SiRF ICD)
    for (int i=0; i<3; i++) {	//for each subframe index 0, 1, 2
        for (int j=0; j<5; j++) { //for each 2 WORDs group
            navW[i*15+j*3] = (gpsSatFrame[satIdx].gpsSatSubframes[i].words[j*2]>>8) & 0xFFFF;
            navW[i*15+j*3+1] = ((gpsSatFrame[satIdx].gpsSatSubframes[i].words[j*2] & 0xFF)<<8) | ((gpsSatFrame[satIdx].gpsSatSubframes[i].words[j*2+1]>>16) & 0xFF);
            navW[i*15+j*3+2] = gpsSatFrame[satIdx].gpsSatSubframes[i].words[j*2+1] & 0xFFFF;
        }
        //the exception is WORD1 (TLM word) of each subframe, whose data are not needed
        navW[i*15] = (unsigned int) satIdx + 1;
        navW[i*15+1] &= 0xFF;
    }
    //check for consistency in the existing data
    sv = navW[0] & 0xFF;		//the SV identification
    if (!((sv == (navW[15] & 0xFF)) && (sv == (navW[30] & 0xFF)))) {
        plog->warning("Different SVs in the frame data");
        return false;
    }
    if (satIdx+1 != sv) plog->warning("Receiver and message data do not match");
    //check for version data consistency
    unsigned int iodcLSB = navW[10] & 0xFF;
    unsigned int iode1 = (navW[15+3]>>8) & 0xFF;
    unsigned int iode2 = navW[30+13] & 0xFF;
    if (!(iode1==iode2 && iode1==iodcLSB)) {
        plog->warning("Different IODs:SV <" + to_string((unsigned long long) sv) +
                      "> IODs <" + to_string((unsigned long long) iodcLSB) +
                      "," + to_string((unsigned long long) iode1) +
                      "," + to_string((unsigned long long) iode2) + ">");
        return false;
    }
/* TBC: to analyze the following code for correctness
	//check for SVhealth
	unsigned int svHealth = (navW[4]>>10) & 0x3F;
	if ((svHealth & 0x20) != 0) {
		sprintf(errorBuff, "Wrong SV %2u health <%02hx>", sv, svHealth);
		plog->info(string(errorBuff));
		return false;
	}
*/
    //fill bom extracting data according SiRF ICD (see A.5 Message # 15: Ephemeris Data)
    //broadcast line 0
    bom[0][0] = navW[11];	//T0C
    bom[0][1] = getTwosComplement(((navW[13] & 0x00FF)<<14) | ((navW[14]>>2) & 0x3FFF), 22);	//Af0
    bom[0][2] = getTwosComplement(((navW[12] & 0x00FF)<<8) | ((navW[13]>>8) & 0x00FF), 16);	//Af1
    bom[0][3] = getTwosComplement((navW[12]>>8) & 0x00FF, 8);								//Af2
    //broadcast line 1
    bom[1][0] = iode1;	//IODE
    bom[1][1] = getTwosComplement(((navW[15+3] & 0x00FF)<<8) | ((navW[15+4]>>8) & 0x00FF), 16);	//Crs
    bom[1][2] = getTwosComplement(((navW[15+4] & 0x00FF)<<8) | ((navW[15+5]>>8) & 0x00FF), 16);	//Delta n
    bom[1][3] = getTwosComplement(((navW[15+5] & 0x00FF)<<24) | ((navW[15+6] & 0xFFFF)<<8) | ((navW[15+7]>>8) & 0x00FF),32);//M0
    //broadcast line 2
    bom[2][0] = getTwosComplement(((navW[15+7] & 0x00FF)<<8) | ((navW[15+8]>>8) & 0x00FF), 16);			//Cuc
    bom[2][1] = (((navW[15+8] & 0x00FF)<<24) | ((navW[15+9] & 0xFFFF)<<8) | ((navW[15+10]>>8) & 0x00FF));	//e
    bom[2][2] = getTwosComplement(((navW[15+10] & 0x00FF)<<8) | ((navW[15+11]>>8) & 0x00FF), 16);		//Cus
    bom[2][3] = ((navW[15+11] & 0x00FF)<<24) | ((navW[15+12] & 0xFFFF)<<8) | ((navW[15+13]>>8) & 0x00FF);	//sqrt(A)
    //broadcast line 3
    bom[3][0] = ((navW[15+13] & 0x00FF)<<8) | ((navW[15+14]>>8) & 0x00FF);	//Toe
    bom[3][1] = getTwosComplement(navW[30+3], 16);						//Cic
    bom[3][2] = getTwosComplement(((navW[30+4] & 0xFFFF)<<16) | (navW[30+5] & 0xFFFF), 32);	//OMEGA
    bom[3][3] = getTwosComplement(navW[30+6], 16);						//CIS
    //broadcast line 4
    bom[4][0] = getTwosComplement(((navW[30+7] & 0xFFFF)<<16) | (navW[30+8] & 0xFFFF), 32);	//i0
    bom[4][1] = getTwosComplement(navW[30+9], 16);											//Crc
    bom[4][2] = getTwosComplement(((navW[30+10] & 0xFFFF)<<16) | (navW[30+11] & 0xFFFF), 32);	//w (omega)
    bom[4][3] = getTwosComplement(((navW[30+12] & 0xFFFF)<<8) | ((navW[30+13]>>8) & 0x00FF), 24);//w dot
    //broadcast line 5
    bom[5][0] = getTwosComplement((navW[30+14]>>2) & 0x03FFF, 14);	//IDOT
    bom[5][1] = (navW[3]>>4) & 0x0003;				//Codes on L2
    bom[5][2] = ((navW[3]>>6) & 0x03FF) + 1024; 	//GPS week#
    bom[5][3] = (navW[4]>>7) & 0x0001;				//L2P data flag
    //broadcast line 6
    bom[6][0] = navW[3] & 0x000F;			//SV accuracy
    bom[6][1] = (navW[4]>>10) & 0x003F;	//SV health
    bom[6][2] = getTwosComplement((navW[10]>>8) & 0x00FF, 8);//TGD
    bom[6][3] = iodcLSB | (navW[4] & 0x0300);		//IODC
    //broadcast line 7
    bom[7][0] = (((navW[1] & 0x00FF)<<9) | ((navW[2]>>7) & 0x01FF)) * 600;	//the 17 MSB of the Zcount in HOW (W2) converted to sec and scaled by 100
    bom[7][1] = (navW[15+14]>>7) & 0x0001;			//Fit flag
    bom[7][2] = 0;		//Spare. Not used
    bom[7][3] = iode2;	//Spare. Used for temporary store of IODE in subframe 3
    return true;
}

/**extractGLOEphemeris extract satellite number, time tag, and ephemeris from the given navigation message string transmitted by GLONASS satellites.
 * The ephemeris extracted are store into a RINEX broadcast orbit like arrangement consisting of  8 lines with 3 ephemeris parameters each.
 * This method stores the mantissa of each satellite ephemeris into a broadcast orbit array, without applying any scale factor.
 *
 * @param satIdx the satellite index
 * @param bom an array of broadcats orbit data containing the mantissa of each satellite ephemeris
 * @param slt the slot number extracted from navigation message
 * @return true if data properly extracted (SV consistency in the channel data, version data consistency and receiver gives confidence on observables), false otherwise
 */
bool GNSSdataFromGRD::extractGLOEphemeris(int satIdx, int (&bom)[8][4], int& slt) {
    //TBC bit numbers, because from Android " each string contains 85 data bits, including the checksum. These bits should be fit into 11 bytes, with MSB first (skip B86-B88)"
    //and currect bit numbers are derived from SiRF & GLONASS ICD
    slt = getBits(gloSatFrame[satIdx].gloSatStrings[3].words, 10, 5);	//slot number (n) in string 4, bits 15-11
    if ((slt < GLO_MINOSN) || (slt > GLO_MAXOSN)) {
        plog->warning("50bps NAV ignored. In string 4 slot number out of range:" + to_string((long long) slt));
        return false;
    }
    int n4 = getBits(gloSatFrame[satIdx].gloSatStrings[4].words, 31, 5);	//four-year interval number N4: bits 36-32 string 5
    int nt = getBits(gloSatFrame[satIdx].gloSatStrings[3].words, 15, 11);	//day number NT: bits 26-16 string 4
    int tb = getBits(gloSatFrame[satIdx].gloSatStrings[1].words, 69, 7);	//time interval index tb: bits 76-70 string 2
    tb *= 15*60;	//convert index to secs
    unsigned int tk =  getBits(gloSatFrame[satIdx].gloSatStrings[0].words, 64, 12);	//Message frame time: bits 76-65 string 1
    int tkSec = (tk >> 7) *60*60;		//hours in tk to secs.
    tkSec += ((tk >> 1) & 0x3F) * 60;	//add min. in tk to secs.
    tkSec += (tk & 0x01) == 0? 0: 30;	//add sec. interval to secs.
    //convert frame time (in GLONASS time) to an UTC instant (use GPS ephemeris for convenience)
    double tTag = getSecsGPSEphe(1996 + (n4-1)*4, 1, nt, 0, 0, (float) tb) - 3*60*60;
    bom[0][0] = (int) tTag;													//Toc
    bom[0][1] = - getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[3].words, 58, 22), 22);	//Clock bias TauN: bits 80-59 string 4
    bom[0][2] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[2].words, 68, 11), 11);	//Relative frequency bias GammaN: bits 79-69 string 3
    bom[0][3] = ((int) getGPStow(tTag) + 518400) % 604800;		//seconds from UTC week start (mon 00:00). Note that GPS week starts sun 00:00.
    bom[1][0] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[0].words, 8, 27), 27);	//Satellite position, X: bits 35-9 string 1
    bom[1][1] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[0].words, 40, 24), 24);	//Satellite velocity, X: bits 64-41 string 1
    bom[1][2] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[0].words, 35, 5), 5);	//Satellite acceleration, X: bits 40-36 string 1
    bom[1][3] = getBits(gloSatFrame[satIdx].gloSatStrings[1].words, 77, 3);					//Satellite health Bn: bits 80-78 string 2
    bom[2][0] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[1].words, 8, 27), 27);	//Satellite position, Y: bits 35-9 string 2
    bom[2][1] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[1].words, 40, 24), 24);	//Satellite velocity, Y: bits 64-41 string 2
    bom[2][2] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[1].words, 35, 5), 5);	//Satellite acceleration, Y: bits 40-36 string 2
    bom[2][3] = carrierFreq[slt-1];										//Frequency number (-7 ... +13)
    bom[3][0] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[2].words, 8, 27), 27);	//Satellite position, Z: bits 35-9 string 3
    bom[3][1] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[2].words, 40, 24), 24);	//Satellite velocity, Z: bits 64-41 string 3
    bom[3][2] = getSigned(getBits(gloSatFrame[satIdx].gloSatStrings[2].words, 35, 5), 5);	//Satellite acceleration, Z: bits 40-36 string 3
    bom[3][3] = getBits(gloSatFrame[satIdx].gloSatStrings[1].words, 48, 5);			//Age of oper. information (days) (E): bits 53-49 string 2
    return true;
}

/**scaleGPSEphemeris apply scale factors to satellite ephemeris mantissas to obtain true satellite ephemirs stored
 * in RINEX broadcast orbit like arrangements.
 * The given navigation data are the mantissa perameters as transmitted in the GPS navigation message, that shall be
 * converted to the true values applying the corresponding scale factor before being used.
 * Also a time tag is obtained from the week and time of week inside the satellite ephemeris.
 *
 * @param bom the mantissas of orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @param bo the orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @return the time tag of the satellite ephemeris
 */
double GNSSdataFromGRD::scaleGPSEphemeris(int (&bom)[8][4], double (&bo)[8][4]) {
    double aDouble;
    int iodc = bom[6][3];
    for(int i=0; i<8; i++) {
        for(int j=0; j<4; j++) {
            if (i==7 && j==1) {		//compute the Fit Interval from fit flag
                if (bom[7][1] == 0) aDouble = 4.0;
                else if (iodc>=240 && iodc<=247) aDouble = 8.0;
                else if (iodc>=248 && iodc<=255) aDouble = 14.0;
                else if (iodc==496) aDouble = 14.0;
                else if (iodc>=497 && iodc<=503) aDouble = 26.0;
                else if (iodc>=1021 && iodc<=1023) aDouble = 26.0;
                else aDouble = 6.0;
                bo[7][1] = aDouble;
            } else if (i==6 && j==0) {	//compute User Range Accuracy value
                bo[6][0] = bom[6][0] < 16? GPS_URA[bom[6][0]] : GPS_URA[15];
            } else if (i==2 && (j==1 || j==3)) {	//e and sqrt(A) are 32 bits unsigned
                bo[2][j] = ((unsigned int) bom[2][j]) * GPS_SCALEFACTOR[2][j];
            } else bo[i][j] = bom[i][j] * GPS_SCALEFACTOR[i][j]; //the rest signed
        }
    }
    return getSecsGPSEphe (
            bom[5][2],	//GPS week#
            bom[0][0] * GPS_SCALEFACTOR[0][0] ); //T0C
}

/**scaleGLOEphemeris apply scale factors to satellite ephemeris mantissas to obtain true satellite ephemirs stored in RINEX broadcast orbit like arrangements.
 * The given navigation data are the mantissa perameters as transmitted in the GLONASS navigation message, that shall be
 * converted to the true values applying the corresponding scale factor before being used.
 * Also a time tag is obtained from the
 *
 * @param bom the mantissas of orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @param bo the orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @return the time tag of the satellite ephemeris
 */
double GNSSdataFromGRD::scaleGLOEphemeris(int (&bom)[8][4], double (&bo)[8][4]) {
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
            bo[i][j] = ((int) bom[i][j]) * GLO_SCALEFACTOR[i][j];
        }
    }
    return (double) bom[0][0];
}

/**gloSatIdx obtains the satellite index from the given OSN or FCN.
 * If the given satellite number is the OSN range index will be in the range 0 to GLO_MAXOSN-1
 * If the satellite number is FCN, index will be in the range GLO_MAXOSN to GLO_MAXFCN - GLO_OSNOFFSET - 1
 *
 * @param stn the satellite number (may be OSN or FCN)
 * @return the satellite index or GLO_MAXSATELLITES if the given stn is not OSN or FCN
 */
int GNSSdataFromGRD::gloSatIdx(int stn) {
    if (stn >= GLO_MINOSN && stn <= GLO_MAXOSN) return (stn - 1);  //normal OSN case, index in range 0 - 23
    if (stn >= GLO_MINFCN && stn <= GLO_MAXFCN) return (stn - GLO_OSNOFFSET - 1); //case FCN, translated to index in range 24 - 37
    return GLO_MAXSATELLITES;
}

/**addSignal adds the given signal (1C,2C,1A,1S,1L,1X, etc) to the given system in the GNSSdataFromGRD object.
 * <p>If the given system does not exist in the RinexData structure, it is added.
 * <p>If the given signal does not exist in the given system, it is added to this system.
 *
 * @param sys is the system identification
 * @param sgnl is the signal name to be added
 * @return true if signal has been added, false otherwise
 */
bool GNSSdataFromGRD::addSignal(char sys, string sgnl) {
    vector<GNSSsystem>::iterator it;
    vector<string> vsgn;
    //check if given sys was already saved
    for (it = systems.begin(); (it != systems.end()) && (it->sysId != sys); ++it);
    if (systems.empty() || (it == systems.end())) {
        vsgn.push_back(sgnl);
        systems.push_back(GNSSsystem(sys, vsgn));
        return true;
    }
    //the sys is in the vector, check if the given sgnl was already saved
    for (vector<string>::iterator itt = it->obsType.begin(); itt != it->obsType.end(); ++itt)
        if ((*itt).compare(sgnl) == 0) return false;
    //save the new signal name for this system
    it->obsType.push_back(sgnl);
    return true;
}

/**setHdSys using existing data on systems and signals obtained from raw data files
 * sets in the rinex object the SYS header record with systems and observation codes
 * available.
 * <p>Note that data available in raw data files would allow computation of pseudorange,
 * carrier phase and doppler observables (not signal stength).
 *
 * @param rinex the RinexData object where SYS erecord will be set.
 */
void GNSSdataFromGRD::setHdSys(RinexData &rinex) {
    vector<string> sgnl;
    try {
        for (vector<GNSSsystem>::iterator it = systems.begin(); it != systems.end(); ++it) {
            for (vector<string>::iterator itt = it->obsType.begin(); itt != it->obsType.end(); ++itt) {
                sgnl.clear();
                sgnl.push_back("C" + (*itt));    //pseudorange signal name
                sgnl.push_back("L" + (*itt));    //carrier phase signal name
                sgnl.push_back("D" + (*itt));    //doppler signal name
                sgnl.push_back("S" + (*itt));    //strength signal name
            }
            rinex.setHdLnData(rinex.SYS, it->sysId, sgnl);
        }
    } catch(string error) {
        plog->severe(error);
    }
}

/**trimBuffer remove from the end of the given c-string the given character to trim.
 * Removal is performed replacing the given character by a null (delimiter of the c-string).
 *
 * @param cstrToTrim a pointer to the begining of the c-string to trim
 * @param unwanted the character to remove
 */
void GNSSdataFromGRD::trimBuffer(char* cstrToTrim, const char* unwanted) {
    char* last = cstrToTrim + strlen(cstrToTrim) - 1;
    while ((last > cstrToTrim) && strchr(unwanted, (int) *last) != NULL) {
        *last = 0;
        last--;
    }
}

/**skipToEOM skips data from input raw data file until END OF MESSAGE is found
 */
void GNSSdataFromGRD::skipToEOM() {
    int c;
    do {
        c = fgetc(grdFile);
    } while ((c != '\n') && (c != EOF));
}

/**llaTOxyz converts geodetic coordinates to Earth-Centered-Earth-Fixed (ECEF).
 *
 * @param lat is the input geodetic latitude in radians
 * @param lon is the input geodetic longitude in radians
 * @param alt is the input geodetic altitude in meters
 * @param x is the output ECEF x coordinate in meters
 * @param y is the output ECEF y coordinate in meters
 * @param z is the output ECEF z coordinate in meters
 */
void GNSSdataFromGRD::llaTOxyz( const double lat, const double lon, const double alt,
                                double &x, double &y, double &z) {
    //rn is the radius of curvature in the prime vertical
    double rn = ECEF_A / sqrt(1.0 - ECEF_E2 * sin(lat) * sin(lat));
    x = (rn + alt) * cos(lat) * cos(lon);    //ECEF x
    y = (rn + alt) * cos(lat) * sin(lon);    //ECEF y
    z = (rn * (1.0 - ECEF_E2) + alt) * sin(lat);  //ECEF z
}

/**getAndSetEpochTime is called just after reading message identifier MT_EPOCH to read data
 * in the rest of the message. Data contained in the message is used to compute the epoch time,
 * which is given as the week number and the time of week (seconds from the
 * beginning of this week.
 * <p> In the current version clock time is referenced to the GPS constellation time frame.
 * <p>The epoch time computed is used to set the current epoch time in the RinexData class.
 * <p>To compute the epoch time the receiver clock bias is taken into account or not, depending on
 * flag applyBias (value set by user). See RINEX V302 document on RCV CLOCK OFFS APPL record:
 * <p>- if applyBias is true, bias computed by receiver after the fix are substracted from the receiver hardware
 * clock to obtain the epoch time.
 * <p>. if applyBias is false, bias are not applied to the receiver hardware.
 *
 * @param rinex the RinexData class where curren epoch ti9me will be set
 * @param tow the time of week, that is, the seconds from the beginning of the current week
 * @param numObs number of MT_SATOBS messages that will follow this one
 * @param msg a message to log
 * @return time of week in nanoseconds from the begining of the current week
 */
long long GNSSdataFromGRD::getAndSetEpochTime(RinexData& rinex, double& tow, int& numObs, string msg) {
    long long tRxGNSS;  //nanos from the beginning of the epoch, corrected with the clock bias or not
    long long tRx;  //nansec. from the beginning of the current week
    long long timeNanos = 0; //is the epochTimeNanos, nanoseconds form GPS ephemeris: 6/1/1980 00:00
    long long fullBiasNanos = 0;
    double biasNanos = 0.0;
    double driftNanos = 0.0;
    double tRxBias;
    int clkDiscont = 0;
    int leapSeconds = 0;
    int eflag = 0;  //0=OK; 1=power failure happened
    int week;   //the current GPS week number
    numObs = 0;
    if (fscanf(grdFile, "%lld;%lld;%lf;%lf;%d;%d;%d", &timeNanos, &fullBiasNanos, &biasNanos, &driftNanos,
               &clkDiscont, &leapSeconds, &numObs) != 7) {
        plog->warning(LOG_MSG_ERRO + "MT_EPOCH params" + LOG_MSG_COUNT);
    }
    //compute time references and set epoch time
    tRxGNSS = timeNanos;
    if (applyBias) tRxGNSS -= fullBiasNanos + (long long) trunc(biasNanos);
    week = (int) (tRxGNSS / NUMBER_NANOSECONDS_WEEK);
    tRx = tRxGNSS % NUMBER_NANOSECONDS_WEEK;
    tow = ((double) tRx) * 1E-9;
    tRxBias = ((double)(fullBiasNanos % NUMBER_NANOSECONDS_WEEK) + biasNanos) * 1E-9;
    if (clockDiscontinuityCount != clkDiscont) {
        eflag = 1;
        clockDiscontinuityCount = clkDiscont;
    }
    rinex.setEpochTime(week, tow, tRxBias, eflag);
    plog->fine(msg + " w=" + to_string(week) + " tow=" + to_string(tow)  + " applyBias:" + (applyBias?string("TRUE"):string("FALSE"))+ LOG_MSG_COUNT);
    return tRx;
}

/**getMsgDescription provides a textual description of the message type (in raw data files or arguments) passed.
 *
 * @param msgt the message type to describe. The description of each message type is in the table msgTblTypes.
 * @return a string with the description found
 */
string GNSSdataFromGRD::getMsgDescription(int msgt) {
    int i = 0;
    while((msgTblTypes[i].type != msgt) && (msgTblTypes[i].type != MT_LAST)) i++;
    return msgTblTypes[i].description;
}

/**getElements extracts lexical elements (tokens) contained in "toExtract" and delimited
 * by any character in "delimiters".
 *
 * @param toExtract the string containing the elements to extract
 * @param delimiters the string containing the characteres used to delimit elements
 * @return a vector with elements found
 */
vector<string> GNSSdataFromGRD::getElements(string toExtract, string delimiters) {
    vector<string> elementsFound;
    size_t eBegin = 0;
    size_t eEnd = 0;
    for (;;) {
        eBegin = toExtract.find_first_not_of(delimiters, eEnd);
        eEnd = toExtract.find_first_of(delimiters, eBegin);
        if (eBegin < eEnd) {
            elementsFound.push_back(toExtract.substr(eBegin, eEnd - eBegin));
        } else break;
    }
    return elementsFound;
}

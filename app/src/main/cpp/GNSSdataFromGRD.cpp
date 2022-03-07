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
    msgCount = 0;
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
    msgCount = 0;
    rewind(grdFile);
}

/**closeInputGRD closes the currently open input GRD file
 */
void GNSSdataFromGRD::closeInputGRD() {
    fclose(grdFile);
}

/**collectHeaderData extracts data from the current ORD or NRD file for the RINEX file header.
 * Also other parameters useful for processing observation or navigation data are collected here.
 * <p>To collect these data the whole file is parsed from begin to end, and lines with message types not containing
 * data useful for header are skipped.
 * <p>Most data are collected only from the first file. Data from MT_SATOBS useful to identify systems and signals
 * being tracked, and from MT_SATNAV_GLONASS_L1_CA related to slot number and carrier frequency are collected from all files.
 * <p>Data collected are saved in the RinexData object passed.
 *
 * @param rinex the RinexData object where header data will be saved
 * @param inFileNum the number of the current input raw data file from a total of inFileTotal. First value = 0
 * @param inFileLast the last number of input file to be processed
 * @return true if header data have been extracted, false otherwise (file cannot be processed)
 */
bool GNSSdataFromGRD::collectHeaderData(RinexData &rinex, int inFileNum = 0, int inFileLast = 0) {
    char msgBuffer[100];
    char smallBuffer[10];
    double dvoid, dvoid2;
    long long llvoid;
    int ivoid;
    string svoid;
    string pgm, runby, date;
    string observer, agency;
    string rcvNum, rcvType, rcvVer;
    vector <string> aVectorStr;
    string logMsg;
    //message parameters in ORD or NRD files
    int msgType;
    char constId;
    int satNum, trackState, phaseState;
    double carrierFrequencyMHz;
    //for data extracted from the navigation message
    bool hasGLOsats;
    bool tofoUnset = true;   //time of first observation not set
    int weekNumber;     //GPS week number without roll over
    string msgEpoch = "Fist epoch";
    rewind(grdFile);
    while ((feof(grdFile) == 0) && (fscanf(grdFile, "%d;", &msgType) == 1)) {
        //there are messages in the raw data file
        msgCount++;
        logMsg = getMsgDescription(msgType);
        switch(msgType) {
            case MT_GRDVER:
                if ((fgets(msgBuffer, sizeof msgBuffer, grdFile) != msgBuffer)
                        || !trimBuffer(msgBuffer, "\r \t\f\v\n")
                        || !processHdData(rinex, msgType, string(msgBuffer))) {
                    plog->severe(logMsg + "CANNOT process this file (.type;version): " + string(msgBuffer));
                    return false;
                }
                //fgets already reads the EOL (skipToEOM not neccesary)!
                continue;
            case MT_DATE:
            case MT_RINEXVER:
            case MT_PGM:
            case MT_RUN_BY:
            case MT_INTERVALMS:
            case MT_SIGU:
            case MT_MARKER_NAME:
            case MT_MARKER_TYPE:
            case MT_OBSERVER:
            case MT_AGENCY:
            case MT_RECNUM:
            case MT_DVTYPE:
            case MT_DVVER:
            case MT_LLA:
            case MT_FIT:
                //they include data directly used for RINEX header lines
                if (fgets(msgBuffer, sizeof msgBuffer, grdFile) == msgBuffer) {
                    trimBuffer(msgBuffer, "\r \t\f\v\n");
                    if (inFileNum == 0) processHdData(rinex, msgType, string(msgBuffer));
                } else plog->warning(logMsg + LOG_MSG_PARERR);
                //fgets already reads the EOL (skipToEOM not neccesary)!
                continue;
            case MT_SATOBS:
                //it includes data used to identify systems and signals being tracked
                memset(smallBuffer, 0, sizeof smallBuffer);     //signal identification to be stored here
                if (fscanf(grdFile, "%c%d;%c%c;%d;%*lld;%*lf;%d;%*lf;%*lf;%lf", &constId, &satNum, smallBuffer, smallBuffer+1, &trackState, &phaseState, &carrierFrequencyMHz) == 7) {
                    if (constId == 'R') satNum = gloOSN(satNum, *smallBuffer, carrierFrequencyMHz, true);
                    //ignore unknown measurements or not having at least a valid pseudorrange or carrier phase
                    if (isKnownMeasur(constId, satNum, *smallBuffer, *(smallBuffer+1))) {
                        if (!isPsAmbiguous(constId, smallBuffer, trackState, dvoid, dvoid2, llvoid) || !isCarrierPhInvalid(constId, smallBuffer, phaseState)) {
                            if (addSignal(constId, string(smallBuffer)))
                                plog->config(logMsg + " added signal " + string(1, constId) + MSG_SPACE + string(smallBuffer));
                        }
                    }
                } else plog->warning(logMsg + LOG_MSG_PARERR);
                break;
            case MT_EPOCH:
                //it includes data used in time related header lines
                collectAndSetEpochTime(rinex, dvoid, ivoid, logMsg + msgEpoch);
                if (tofoUnset && inFileNum == 0) {
                    //set Time of Firts and Last Observation
                    rinex.setHdLnData(rinex.TOFO);
                    tofoUnset = false;
                    rinex.setHdLnData(rinex.TOLO);
                    //compute number of roll overs occurred at current epoch time for different constellations
                    rinex.getEpochTime(weekNumber, dvoid, dvoid2, ivoid);
                    nGPSrollOver = weekNumber / 1024;
                    nGALrollOver = (weekNumber - 1024) / 4096;  //GALT starts at first GPST roll over (22/8/1999 00:00:00 GPST)
                    nBDSrollOver = (weekNumber - 1356) / 8192;  //1356: week number of Jan 1, 2006 (BDST start) = 1/1/2006 00:00:14 GPST
                    msgEpoch = "Epoch ";
                } else {
                    rinex.setHdLnData(rinex.TOLO);
                }
                break;
            case MT_SATNAV_GPS_L1_CA:
                //it includes data used in corrections (ionospheric, time, etc.) header lines
                if (collectGPSL1CACorrections(rinex, msgType)) {
                    rinex.setHdLnData(RinexData::SYS, 'G', aVectorStr);
                }
                break;
            case MT_SATNAV_GPS_L5_C:
            case MT_SATNAV_GPS_C2:
            case MT_SATNAV_GPS_L2_C:
                //TODO extract data for header lines from these navigation message signals
                rinex.setHdLnData(RinexData::SYS, 'G', aVectorStr);
                break;
            case MT_SATNAV_GLONASS_L1_CA:
                //it includes data used in corrections (ionospheric, time, etc.) header lines, and identify satellite OSN
                if (collectGLOL1CACorrections(rinex, msgType)) {
                    rinex.setHdLnData(RinexData::SYS, 'R', aVectorStr);
                }
                break;
            case MT_SATNAV_GALILEO_INAV:
                //it includes data used in corrections (ionospheric, time, etc.) header lines
                if (collectGALINCorrections(rinex, msgType)) {
                    rinex.setHdLnData(RinexData::SYS, 'E', aVectorStr);
                }
                break;
            case MT_SATNAV_GALILEO_FNAV:
                //TODO extract data for header lines from this navigation message signal
                rinex.setHdLnData(RinexData::SYS, 'E', aVectorStr);
                break;
            case MT_SATNAV_BEIDOU_D1:
                //it includes data used in corrections (ionospheric, time, etc.) header lines
                if (collectBDSD1Corrections(rinex, msgType)) {
                    rinex.setHdLnData(RinexData::SYS, 'C', aVectorStr);
                }
                break;
            case MT_SATNAV_BEIDOU_D2:
                //TODO extract data for header lines from this navigation message signal
                rinex.setHdLnData(RinexData::SYS, 'C', aVectorStr);
                break;
            default:
                plog->warning(logMsg + to_string(msgType));
                break;
        }
        skipToEOM();
    }
    if (inFileNum == inFileLast) {
        setHdSys(rinex);
        processFilterData(rinex);
        hasGLOsats = false;
        for (int i=0; rinex.getHdLnData(RinexData::SYS, constId, aVectorStr, i); i++) {
            //set empty PHSH records because Android does not provide specific data on this subject
            aVectorStr.clear();
            rinex.setHdLnData(RinexData::PHSH, constId, string(), 0.0, aVectorStr);
            if (constId == 'R') hasGLOsats = true;
        }
        if (hasGLOsats) {
            //set empty GLPHS records because Android does not provides data on this subject
            rinex.setHdLnData(RinexData::GLPHS, "C1C", 0.0);
            rinex.setHdLnData(RinexData::GLPHS, "C1P", 0.0);
            rinex.setHdLnData(RinexData::GLPHS, "C2C", 0.0);
            rinex.setHdLnData(RinexData::GLPHS, "C2P", 0.0);
            //set GLSLT data for the existing slots
            //?for (int i=0; i<GLO_MAXSATELLITES; i++) {
            for (int i=0; i<GLO_MAXOSN; i++) {
                if ((glonassOSN_FCN[i].osn != 0) && glonassOSN_FCN[i].fcnSet) {
                    rinex.setHdLnData(RinexData::GLSLT, glonassOSN_FCN[i].osn, glonassOSN_FCN[i].fcn);
                }
            }
            //log the OSN - FCN table
            plog->config("Table from GLONASS almanacs [Sat, nA(OSN), HnA(FCN)]:");
            ivoid = 1;      //offset to obtain satellite number from table index
            for (int i=0; i<GLO_MAXSATELLITES; i++) {
                if (i == GLO_MAXOSN) ivoid = GLO_FCN2OSN;
                logMsg = "R" + to_string(i+ivoid) + MSG_COMMA;
                if (glonassOSN_FCN[i].osn != 0) logMsg += to_string(glonassOSN_FCN[i].osn);
                logMsg += MSG_COMMA;
                if (glonassOSN_FCN[i].fcnSet) logMsg += to_string(glonassOSN_FCN[i].fcn);
                plog->config(logMsg);
            }
        }
    }
    return true;
}

/**collectEpochObsData extracts observation and time data from observation raw data (ORD) file messages for one epoch.
 *<p>Epoch RINEX data are contained in a sequence of messages in the form: MT_EPOCH {MT_SATOBS}.
 *<p>The epoch starts with a MT_EPOCH message. It contains clock data for the current epoch, including its time,
 * and the number of MT_SATOBS that would follow.
 *<p>Each MT_SATOBS message contains observables for a given satellite (in each epoch the receiver sends  a MT_SATOBS
 * message for each satellite being tracked).
 * An unexpected end of epoch occurs when the number of MT_SATOBS messages does not correspond with what has been
 * stated in MT_EPOCH. This event would be logged at warning level.
 *<p>This method acquires data reading messages recorded in the raw data file according to the above sequence and
 * computes the obsevables from the acquired raw data. These data are saved in the RINEX data structure for further
 * generation/printing of RINEX observation records in the observation file.
 * The method ends when it is read the last message of the current epoch.
 *<p>Other messages in the input file different from  MT_EPOCH ot MT_SATOBS are ignored.
 *
 * @param rinex the RinexData object where data got from receiver will be placed
 * @return true when data from all observation messages of an epoch have been acquired, false otherwise (End Of File reached)
 */
bool GNSSdataFromGRD::collectEpochObsData(RinexData &rinex) {
    //variables to get data from ORD observation records
    int msgType;
    char constellId;
    int satNum;
    char signalId[4] = {0};
    int synchState;
    long long tTx = 0;  //the satellite transmitted clock in nanosec. (from receivedSatTimeNanos given by Android I/F)
    long long tTxUncert;
    double timeOffsetNanos;
    int carrierPhaseState;
    double carrierPhase, cn0db, carrierFrequencyMHz;
    double psRangeRate, psRangeRateUncert;
    //variables to obtain observables
    //This app version assumes for tRxGNSS the GPS time reference (tRxGNSS is here tRxGPS)
    double tRx = 0.0;    //the receiver clock in nanosececonds from the beginning of the current GPS week
    double tRxGNSS = 0;  //the receiver clock for the GNSS system of the satellite being processed
    double tow = 0;         //time of week in seconds from the beginning of the current GPS week
    double pseudorange = 0.0;
    double dopplerShift = 0.0;
    int lli = 0;    //loss o lock indicator as per RINEX V3.04
    int sn_rnx = 0;   //signal strength as per RINEX V3.04
    int numMeasur = 0; //number of satellite measurements in current epoch
    bool psAmbiguous = false;
    bool phInvalid = false;
    while (fscanf(grdFile, "%d;", &msgType) == 1) {
        msgCount++;
        switch(msgType) {
            case MT_EPOCH:
                if (numMeasur > 0) plog->warning(getMsgDescription(msgType) + "Few MT_SATOBS in epoch");
                tRx = collectAndSetEpochTime(rinex, tow, numMeasur, getMsgDescription(msgType) + "Epoch");
                break;
            case MT_SATOBS:
                if (numMeasur <= 0) {
                    plog->warning(getMsgDescription(msgType) + "MT_SATOBS before MT_EPOCH");
                    break;
                }
                numMeasur--;    //an MT_SATOBS has been read, get its parameters
                if (fscanf(grdFile, "%c%d;%c%c;%d;%lld;%lf;%d;%lf;%lf;%lf;%lf;%lf;%lld",
                           &constellId, &satNum, signalId+1, signalId+2,
                           &synchState, &tTx, &timeOffsetNanos,
                           &carrierPhaseState, &carrierPhase,
                           &cn0db,  &carrierFrequencyMHz,
                           &psRangeRate, &psRangeRateUncert, &tTxUncert) != 14) {
                    plog->warning(getMsgDescription(msgType) + "MT_SATOBS params");
                    break;
                }
                //solve the issue of Glonass satellite number
                if (constellId == 'R') satNum = gloOSN(satNum);
                if (isKnownMeasur(constellId, satNum, signalId[1], signalId[2])) {
                    psAmbiguous = isPsAmbiguous(constellId, signalId+1, synchState, tRx, tRxGNSS, tTx);
                    phInvalid = isCarrierPhInvalid(constellId, signalId, carrierPhaseState);
                    if (!psAmbiguous || !phInvalid) {
                        //from data available compute signal to noise RINEX index
                        sn_rnx = (int) (cn0db / 6);
                        if (sn_rnx < 1) sn_rnx = 1;
                        else if (sn_rnx > 9) sn_rnx = 1;
                        //set and comupute pseudorrange values. Computation depends on tracking state and constellation time frame
                        signalId[0] = 'C';
                        pseudorange = (tRxGNSS - (double) tTx - timeOffsetNanos) * SPEED_OF_LIGTH_MxNS;
                        if (psAmbiguous || pseudorange < 0) pseudorange = 0.0;
                        rinex.saveObsData(constellId, satNum, string(signalId), pseudorange, 0, sn_rnx, tow);
                        //set carrier phase values and LLI
                        signalId[0] = 'L';
                        lli = 0;    //valid or unkown by default
                        if (phInvalid) {
                            carrierPhase = 0.0;   //invalid carrier phase
                        } else {
                            if ((carrierPhaseState & ADR_ST_CYCLE_SLIP) != 0) lli |= 0x01;  //cycle slip detected
                            if ((carrierPhaseState & ADR_ST_RESET) != 0) lli |= 0x01;   //reset detected
                            if ((carrierPhaseState & ADR_ST_HALF_CYCLE_RESOLVED) != 0) lli |= 0x01;
                        }
                        //phase, given in meters, shall be converted to full cycles
                        //TODO to analyse taking into account apply bias and half cycle
                        carrierPhase *= carrierFrequencyMHz * WLFACTOR;
                        rinex.saveObsData(constellId, satNum, string(signalId), carrierPhase, lli, sn_rnx, tow);
                        //set doppler values
                        signalId[0] = 'D';
                        dopplerShift = - psRangeRate * carrierFrequencyMHz * DOPPLER_FACTOR;
                        rinex.saveObsData(constellId, satNum, string(signalId), dopplerShift, 0, sn_rnx, tow);
                        //set signal to noise values
                        signalId[0] = 'S';
                        rinex.saveObsData(constellId, satNum, string(signalId), cn0db, 0, sn_rnx, tow);
                        plog->finer(getMsgDescription(msgType) + string(1, constellId) + to_string(satNum) + MSG_SPACE +
                                   string(signalId+1) + MSG_SPACE +
                                   to_string(pseudorange) + MSG_SPACE + to_string(carrierPhase) + MSG_SPACE +
                                   to_string(dopplerShift) + MSG_SPACE + to_string(cn0db));
                    } else {
                        plog->fine(getMsgDescription(msgType) + string(1, constellId) + to_string(satNum) + MSG_SPACE +
                                   string(signalId+1) + LOG_MSG_INVM);
                    }
                } else plog->warning(getMsgDescription(msgType) + string(1, constellId) + to_string(satNum) + MSG_SPACE +
                                     string(signalId+1) + MSG_SPACE + LOG_MSG_UNK);
                if (numMeasur <= 0) {
                    skipToEOM();
                    return true;
                }
                break;
            case MT_SATNAV_GPS_L1_CA:
            case MT_SATNAV_GLONASS_L1_CA:
            case MT_SATNAV_GALILEO_FNAV:
            case MT_SATNAV_BEIDOU_D1:
            case MT_SATNAV_GPS_L5_C:
                plog->warning(getMsgDescription(msgType) + LOG_MSG_NINO);
            default:
                break;
        }
        skipToEOM();
    }
    return false;
}

/**collectNavData iterates over the input raw data file extracting navigation messages to process their data.
 * Data extracted are saved in a RinexData object for futher printing in a navigation RINEX file.
 * It is assumed that grdFile file type and version are the correct ones.
 *
 * @param rinex the RinexData object where data got from receiver will be placed
 * @return true when navigation data from at least one epoch messages have been acquired, false otherwise
 */
bool GNSSdataFromGRD::collectNavData(RinexData &rinex) {
    int msgType;
    bool acquiredNavData = false;
    msgCount = 0;
    while (fscanf(grdFile, "%d;", &msgType) == 1) {
        msgCount++;
        switch(msgType) {
            case MT_SATNAV_GPS_L1_CA:
                acquiredNavData |= collectGPSL1CAEphemeris(rinex, msgType);
                break;
            case MT_SATNAV_GLONASS_L1_CA:
                acquiredNavData |= collectGLOL1CAEphemeris(rinex, msgType);
                break;
            case MT_SATNAV_GALILEO_INAV:
                acquiredNavData |= collectGALINEphemeris(rinex, msgType);
                break;
            case MT_SATNAV_BEIDOU_D1:
                acquiredNavData |= collectBDSD1Ephemeris(rinex, msgType);
                break;
            case MT_SATNAV_GPS_L5_C:
            case MT_SATNAV_GPS_C2:
            case MT_SATNAV_GPS_L2_C:
            case MT_SATNAV_GALILEO_FNAV:
            case MT_SATNAV_BEIDOU_D2:
                plog->warning(getMsgDescription(msgType) + MSG_NOT_IMPL + LOG_MSG_NAVIG);
                break;
            case MT_EPOCH:
                break;
            case MT_SATOBS:
                plog->warning(getMsgDescription(msgType) + LOG_MSG_NONI);
            default:
                break;
        }
        skipToEOM();
    }
    return acquiredNavData;
}

/**processHdData sets the Rinex header record data from the contents of the given message.
 * Each message type contains data to be processed and stored in the corresponding header record.
 *
 * @param rinex the RinexData object which header data will be updated
 * @param msgType the raw data file message type
 * @param msgContent the content of the message with data to be extracted
 * @return true when data has been processed without error, false otherwise
 */
bool GNSSdataFromGRD::processHdData(RinexData &rinex, int msgType, string msgContent) {
    double da, db, dc;
    double x, y, z;
    string msgError;
    vector<string> selElements;
    size_t position;
    double dvoid = 0.0;
    string svoid = string();
    plog->config(getMsgDescription(msgType) + msgContent);
    try {
        switch(msgType) {
            case MT_GRDVER:
                position = msgContent.find(';');
                if ((position != string::npos)
                    && isGoodGRDver(msgContent.substr(0, position), stoi(msgContent.substr(position+1, string::npos)))) return true;
                break;
            case MT_SITE:
                plog->finer(getMsgDescription(msgType) + " currently ignored");
                return true;
            case MT_PGM:
                return rinex.setHdLnData(rinex.RUNBY, msgContent, svoid, svoid);
            case MT_DVTYPE:
                return rinex.setHdLnData(rinex.RECEIVER, svoid, msgContent, svoid);
            case MT_DVVER:
                return rinex.setHdLnData(rinex.RECEIVER, svoid, svoid, msgContent);
            case MT_LLA:
                if (sscanf(msgContent.c_str(), "%lf;%lf;%lf", &da, &db, &dc) == 3) {
                    llaTOxyz(da * dgrToRads, db * dgrToRads, dc, x, y, z);
                    return rinex.setHdLnData(rinex.APPXYZ, x, y, z);
                }
                plog->warning(getMsgDescription(msgType) + LOG_MSG_PARERR);
                return false;
            case MT_DATE:      //Date of the file
                return rinex.setHdLnData(rinex.RUNBY, svoid, svoid, msgContent);
            case MT_INTERVALMS:
                return rinex.setHdLnData(rinex.INT, stod(msgContent) / 1000., dvoid, dvoid);
            case MT_SIGU:
                return rinex.setHdLnData(rinex.SIGU, msgContent, svoid, svoid);
            case MT_RINEXVER:
                return rinex.setHdLnData(rinex.VERSION, stod(msgContent), dvoid, dvoid);
            case MT_RUN_BY:
                return rinex.setHdLnData(rinex.RUNBY, svoid, msgContent, svoid);
            case MT_MARKER_NAME:
                return rinex.setHdLnData(rinex.MRKNAME, msgContent, svoid, svoid);
            case MT_MARKER_TYPE:
                return rinex.setHdLnData(rinex.MRKTYPE, msgContent, svoid, svoid);
            case MT_OBSERVER:
                return rinex.setHdLnData(rinex.AGENCY, msgContent, svoid, svoid);
            case MT_AGENCY:
                return rinex.setHdLnData(rinex.AGENCY, svoid, msgContent, svoid);
            case MT_RECNUM:
                return rinex.setHdLnData(rinex.RECEIVER, msgContent, svoid, svoid);
            case MT_COMMENT:
                return rinex.setHdLnData(rinex.COMM, rinex.RUNBY, msgContent);
            case MT_MARKER_NUM:
                return rinex.setHdLnData(rinex.MRKNUMBER, msgContent, svoid, svoid);
            case MT_CLKOFFS:
                clkoffset = stoi(msgContent);
                rinex.setHdLnData(rinex.CLKOFFS, clkoffset);
                applyBias = clkoffset == 1;
                plog->config(getMsgDescription(msgType) + " applyBias:" + (applyBias?string("TRUE"):string("FALSE")));
                return true;
            case MT_FIT:
                if (msgContent.find("TRUE") != string::npos) fitInterval = true;
                else fitInterval = false;
                return true;
            case MT_LOGLEVEL:
                plog->setLevel(msgContent);
                return true;
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
                        plog->warning(getMsgDescription(msgType) + LOG_MSG_UNKSELSYS + (*it));
                        selElements.erase(it);
                    }
                }
                selSatellites.insert(selSatellites.end(), selElements.begin(), selElements.end());
                return true;
            case MT_SATELLITES:
                selElements = getElements(msgContent, "[],;.:- ");
                selSatellites.insert(selSatellites.end(), selElements.begin(), selElements.end());
                return true;
            case MT_OBSERVABLES:
                selObservables = getElements(msgContent, "[], ");
                return true;
            default:
                plog->warning(getMsgDescription(msgType) + to_string(msgType));
                break;
        }
    } catch (string error) {
        plog->severe(error + LOG_MSG_COUNT);
    }
    return false;
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

/**setInitValues set initial values in class variables and tables containig f.e. conversion parameters
 * used to translate scaled normalized GPS message data to values in actual units.
 * <b>Called by the construtors
 */
void GNSSdataFromGRD::setInitValues() {
    ordVersion = 0;
    nrdVersion = 0;
    msgCount = 0;
    clkoffset = 0;
    applyBias = false;
    fitInterval = false;
    //default values for roll overs
    nGPSrollOver = 2;
    nGALrollOver = 0;
    nBDSrollOver = 0;
    //scale factors to apply to ephemeris
    //set default values
    double COMMON_SCALEFACTOR[BO_LINSTOTAL][BO_MAXCOLS]; //the scale factors to apply to GPS, GAL and BDS broadcast orbit data to obtain ephemeris
    for (int i = 0; i < BO_LINSTOTAL; i++) {
        for (int j = 0; j < BO_MAXCOLS; j++) {
            COMMON_SCALEFACTOR[i][j] = 1.0;
        }
    }
    //factors common to GPS, Glonass, Galileo or Beidou?
    //SV clock data have specific factors for each constellation
    //broadcast orbit 1
    COMMON_SCALEFACTOR[1][1] = pow(2.0, -5.0);              //Crs
    COMMON_SCALEFACTOR[1][2] = pow(2.0, -43.0) * ThisPI;	//Delta N
    COMMON_SCALEFACTOR[1][3] = pow(2.0, -31.0) * ThisPI;	//M0
    //broadcast orbit 2
    COMMON_SCALEFACTOR[2][0] = pow(2.0, -29.0);	//Cuc
    COMMON_SCALEFACTOR[2][1] = pow(2.0, -33.0);	//e
    COMMON_SCALEFACTOR[2][2] = pow(2.0, -29.0);	//Cus
    COMMON_SCALEFACTOR[2][3] = pow(2.0, -19.0);	//sqrt(A) ????
    //broadcast orbit 3
    COMMON_SCALEFACTOR[3][1] = pow(2.0, -29.0);	    //Cic
    COMMON_SCALEFACTOR[3][2] = pow(2.0, -31.0) * ThisPI;	//Omega0
    COMMON_SCALEFACTOR[3][3] = pow(2.0, -29.0);	    //Cis
    //broadcast orbit 4
    COMMON_SCALEFACTOR[4][0] = pow(2.0, -31.0) * ThisPI;	//i0
    COMMON_SCALEFACTOR[4][1] = pow(2.0, -5.0);              //Crc
    COMMON_SCALEFACTOR[4][2] = pow(2.0, -31.0) * ThisPI;	//w
    COMMON_SCALEFACTOR[4][3] = pow(2.0, -43.0) * ThisPI;	//w dot
    //broadcast orbit 5
    COMMON_SCALEFACTOR[5][0] = pow(2.0, -43.0) * ThisPI;	//Idot
    //Iono scale factors
    COMMON_SCALEFACTOR[BO_LIN_IONOA][0] = pow(2.0, -30);   //Iono alfa0
    COMMON_SCALEFACTOR[BO_LIN_IONOA][1] = pow(2.0, -27);   //Iono alfa1
    COMMON_SCALEFACTOR[BO_LIN_IONOA][2] = pow(2.0, -24);   //Iono alfa2
    COMMON_SCALEFACTOR[BO_LIN_IONOA][3] = pow(2.0, -24);   //Iono alfa3
    COMMON_SCALEFACTOR[BO_LIN_IONOB][0] = pow(2.0, 11);   //Iono beta0
    COMMON_SCALEFACTOR[BO_LIN_IONOB][1] = pow(2.0, 14);   //Iono beta1
    COMMON_SCALEFACTOR[BO_LIN_IONOB][2] = pow(2.0, 16);   //Iono beta2
    COMMON_SCALEFACTOR[BO_LIN_IONOB][3] = pow(2.0, 16);   //Iono beta3
    //Time corrections common scale factors
    COMMON_SCALEFACTOR[BO_LIN_TIMEU][0] = pow(2.0, -30);   //TCORR A0UTC
    COMMON_SCALEFACTOR[BO_LIN_TIMEU][1] = pow(2.0, -50);   //TCORR A1UTC
    COMMON_SCALEFACTOR[BO_LIN_TIMEU][3] = 1.0;      //WNt scale factor
    COMMON_SCALEFACTOR[BO_LIN_TIMEG][2] = 1.0;
    COMMON_SCALEFACTOR[BO_LIN_TIMEG][3] = 1.0;
    //set scale factors for GPS
    memcpy(GPS_SCALEFACTOR, COMMON_SCALEFACTOR, sizeof(GPS_SCALEFACTOR));
    //SV clock data
    GPS_SCALEFACTOR[0][0] = pow(2.0, 4.0);		//T0c
    GPS_SCALEFACTOR[0][1] = pow(2.0, -31.0);	//Af0: SV clock bias
    GPS_SCALEFACTOR[0][2] = pow(2.0, -43.0);	//Af1: SV clock drift
    GPS_SCALEFACTOR[0][3] = pow(2.0, -55.0);	//Af2: SV clock drift rate GAL
    //broadcast orbit 3
    GPS_SCALEFACTOR[3][0] = pow(2.0, 4.0);		//TOE
    //broadcast orbit 6
    GPS_SCALEFACTOR[6][2] = pow(2.0, -31.0);	//TGD
    //broadcast orbit 7
    GPS_SCALEFACTOR[7][0] = 0.01;	//Transmission time of message in sec x 100
    GPS_SCALEFACTOR[7][2] = 0.0;	//Spare
    GPS_SCALEFACTOR[7][3] = 0.0;	//Spare
    //GPST - UTC factors
    GPS_SCALEFACTOR[BO_LIN_TIMEU][2] = pow(2.0, 12);    //t0t scale factor

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
    //set factors for Galileo
    memcpy(GAL_SCALEFACTOR, COMMON_SCALEFACTOR, sizeof(GAL_SCALEFACTOR));
    //SV clock data
    GAL_SCALEFACTOR[0][0] = 60.;		        //T0c
    GAL_SCALEFACTOR[0][1] = pow(2.0, -34.0);	//Af0: SV clock bias
    GAL_SCALEFACTOR[0][2] = pow(2.0, -46.0);	//Af1: SV clock drift
    GAL_SCALEFACTOR[0][3] = pow(2.0, -59.0);	//Af2: SV clock drift rate
    //broadcast orbit 3
    GAL_SCALEFACTOR[3][0] = 60.;		//TOE
    //broadcast orbit 5
    GAL_SCALEFACTOR[5][3] = 0.0;		//spare
    //broadcast orbit 6
    GAL_SCALEFACTOR[6][2] = pow(2.0, -32.0);	//BGD E5a / E1
    GAL_SCALEFACTOR[6][3] = pow(2.0, -32.0);	//BGD E5b / E1
    //broadcast orbit 7
    //GAL_SCALEFACTOR[7][0] = 1.0;	//Transmission time of message
    //GAL_SCALEFACTOR[7][1] = 0.0;	//Spare
    //GAL_SCALEFACTOR[7][2] = 0.0;	//Spare
    //GAL_SCALEFACTOR[7][3] = 0.0;	//Spare
    //other data stored in bom
    GAL_SCALEFACTOR[BO_LIN_IONOA][0] = pow(2.0, -2);   //Iono Ai0
    GAL_SCALEFACTOR[BO_LIN_IONOA][1] = pow(2.0, -8);   //Iono Ai1
    GAL_SCALEFACTOR[BO_LIN_IONOA][2] = pow(2.0, -15);  //Iono Ai2
    GAL_SCALEFACTOR[BO_LIN_IONOA][3] = 0.0;           //Iono blank
    GAL_SCALEFACTOR[BO_LIN_TIMEU][2] = 3600;          //GST-UTC t0t
    GAL_SCALEFACTOR[BO_LIN_TIMEG][0] = pow(2.0, -35);    //GST - GPS time corr. A0GPS
    GAL_SCALEFACTOR[BO_LIN_TIMEG][1] = pow(2.0, -51);    //GST - GPS time corr. A1GPS
    GAL_SCALEFACTOR[BO_LIN_TIMEG][2] = 3600;          //GST-GPS t0G
    //Glonass scale factors
    //SV clock data
    GLO_SCALEFACTOR[0][0] = 1.;		            //T0c
    GLO_SCALEFACTOR[0][1] = pow(2.0, -30.0);	//-TauN
    GLO_SCALEFACTOR[0][2] = pow(2.0, -40.0);	//+GammaN
    GLO_SCALEFACTOR[0][3] = 1.;					//Message frame time
    //broadcast orbit 1
    GLO_SCALEFACTOR[1][0] = pow(2.0, -11.0);	//X
    GLO_SCALEFACTOR[1][1] = pow(2.0, -20.0);	//Vel X
    GLO_SCALEFACTOR[1][2] = pow(2.0, -30.0);	//Accel X
    GLO_SCALEFACTOR[1][3] = 1.;					//SV Health
    //broadcast orbit 2
    GLO_SCALEFACTOR[2][0] = pow(2.0, -11.0);	//Y
    GLO_SCALEFACTOR[2][1] = pow(2.0, -20.0);	//Vel Y
    GLO_SCALEFACTOR[2][2] = pow(2.0, -30.0);	//Accel Y
    GLO_SCALEFACTOR[2][3] = 1.;					//Frequency number
    //broadcast orbit 3
    GLO_SCALEFACTOR[3][0] = pow(2.0, -11.0);	//Z
    GLO_SCALEFACTOR[3][1] = pow(2.0, -20.0);	//Vel Z
    GLO_SCALEFACTOR[3][2] = pow(2.0, -30.0);	//Accel Z
    GLO_SCALEFACTOR[3][3] = 1.;					//Age of oper.
    //other data stored in bom
    GLO_SCALEFACTOR[BO_LIN_TIMEU][0] = pow(2.0, -31);    //TauC
    GLO_SCALEFACTOR[BO_LIN_TIMEU][1] = 0.0;
    GLO_SCALEFACTOR[BO_LIN_TIMEU][2] = 0.0;
    GLO_SCALEFACTOR[BO_LIN_TIMEU][3] = 0.0;
    GLO_SCALEFACTOR[BO_LIN_TIMEG][0] = pow(2.0, -30);    //TauGPS
    GLO_SCALEFACTOR[BO_LIN_TIMEG][1] = 0.0;
    GLO_SCALEFACTOR[BO_LIN_TIMEG][2] = 0.0;
    GLO_SCALEFACTOR[BO_LIN_TIMEG][3] = 0.0;
    //set scale factors for BDS
    memcpy(BDS_SCALEFACTOR, COMMON_SCALEFACTOR, sizeof(BDS_SCALEFACTOR));
    //SV clock data
    BDS_SCALEFACTOR[0][0] = pow(2.0, 3.0);		//T0c
    BDS_SCALEFACTOR[0][1] = pow(2.0, -33.0);	//Af0: SV clock bias
    BDS_SCALEFACTOR[0][2] = pow(2.0, -50.0);	//Af1: SV clock drift
    BDS_SCALEFACTOR[0][3] = pow(2.0, -66.0);	//Af2: SV clock drift rate GAL
    //broadcast orbit 1
    BDS_SCALEFACTOR[1][1] = pow(2.0, -6.0);     //Crs
    //broadcast orbit 2
    BDS_SCALEFACTOR[2][0] = pow(2.0, -31.0);	//Cuc
    BDS_SCALEFACTOR[2][2] = pow(2.0, -31.0);	//Cus
    //broadcast orbit 3
    BDS_SCALEFACTOR[3][0] = pow(2.0, 3.0);	    //T0e
    BDS_SCALEFACTOR[3][1] = pow(2.0, -31.0);	//Cic
    BDS_SCALEFACTOR[3][3] = pow(2.0, -31.0);	//Cis
    //broadcast orbit 4
    BDS_SCALEFACTOR[4][1] = pow(2.0, -6.0);     //Crc
    //broadcast orbit 5
    BDS_SCALEFACTOR[5][1] = 0.0;	//Spare
    BDS_SCALEFACTOR[5][3] = 0.0;	//Spare
    //broadcast orbit 6
    //broadcast orbit 7
    BDS_SCALEFACTOR[7][2] = 0.0;	//Spare
    BDS_SCALEFACTOR[7][3] = 0.0;	//Spare
    //other data stored in bom
    BDS_SCALEFACTOR[BO_LIN_TIMEG][0] = BDS_SCALEFACTOR[BO_LIN_TIMEG][1] = 1.0E-10;    //A0GPS, A1GPS
    //BDS_URA table to obtain in meters the value associated to the satellite accuracy index
    BDS_URA[0] = 2.4;
    BDS_URA[1] = 3.4;
    BDS_URA[2] = 4.85;
    BDS_URA[3] = 6.85;
    BDS_URA[4] = 9.65;
    BDS_URA[5] = 13.65;
    BDS_URA[6] = 24.0;
    BDS_URA[7] = 48.0;
    BDS_URA[8] = 96.0;
    BDS_URA[9] = 192,0;
    BDS_URA[10] = 384,0;
    BDS_URA[11] = 768,0;
    BDS_URA[12] = 1536.0;
    BDS_URA[13] = 3072.0;
    BDS_URA[14] = 6144.0;
    BDS_URA[15] = 6144.0;
    //set tables to 0
    memset(gpsSatFrame, 0, sizeof(gpsSatFrame));
    memset(gloSatFrame, 0, sizeof(gloSatFrame));
    memset(glonassOSN_FCN, 0, sizeof(glonassOSN_FCN));
    for(int i=0; i<GLO_MAXSATELLITES; i++) glonassOSN_FCN[i].fcnSet = false;
    memset(nAhnA, 0, sizeof(nAhnA));
    memset(galInavSatFrame, 0, sizeof(galInavSatFrame));
}


//methods for GPS L1 CA message processing
//A macro to get bit position in a subframe from the bit position (BITNUMBER) in the message subframe stated in the GPS ICD
//Each GPS 30 bits message word is stored in an uint32_t (aligned to the rigth)
//Each GPS subframe, comprising 10 words, is tored in an stream of 10 uint32_t
//BITNUMBER is the position in the subframe, GPSL1CA_BIT gets the position in the stream
#define GPSL1CA_BIT(BITNUMBER) ((BITNUMBER-1)/30*32 + (BITNUMBER-1)%30 + 2)

/**collectGPSL1CAEphemeris gets GPS L1 C/A navigation data from a raw message and store them to generate RINEX navigation files.
 * Satellite navigation messages are temporary stored until all needed subframes for the current satellite and epoch have been received.
 * When completed, satellite ephemeris are extracted from subframes, scaled to working units, and stored into "broadcast orbit lines"
 * of the RinexData object.
 * For GPS L1 C/A each subframe of the navigation message contains 10 30-bit words.
 * Each word (30 bits) should be fit into the last 30 bits in a 4-byte word (skip B31 and B32), with MSB first,
 * for a total of 40 bytes, covering a time period of 6, 6, and 0.6 seconds, respectively.
 * Only have interest subframes: 1,2,3 & page 18 of subframe 4 (pgID = 56 in GPS ICD Table 20-V)
 *
 * @param rinex	the class instance where data are stored
 * @param msgType the true type of the message
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectGPSL1CAEphemeris(RinexData &rinex, int msgType) {
    char constId;   //the constellation identifier this satellite belongs
    int satNum;		//the satellite number this navigation message belongssatt
    int sfrmNum;    //navigation message subframe number
    int pageNum;    //navigation message page number
    int bom[BO_LINSTOTAL][BO_MAXCOLS];		//a RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
    double bo[BO_LINSTOTAL][BO_MAXCOLS];	//the RINEX broadcats orbit arrangement for satellite ephemeris
    double tTag;		//the time tag for ephemeris data
    GPSFrameData *pframe;
    string logMsg = getMsgDescription(msgType);
    //read MT_SATNAV_GPS_L1_CA message data
    if (!readGPSL1CANavMsg(constId, satNum, sfrmNum, pageNum, logMsg)) return false;
    pframe = &gpsSatFrame[satNum - 1];
    //check if all ephemeris have been received, that is:
    //-subframes 1, 2, and 3 have data
    //-and their data belong belong to the same Issue Of Data (IOD)
    bool allRec = pframe->hasData;
    for (int i = 0; allRec && i < 3; ++i) allRec = allRec && pframe->gpsSatSubframes[i].hasData;
    if (allRec) {
        logMsg += LOG_MSG_FRM;
        //IODC (8LSB in subframe 1) must be equal to IODE in subframe 2 and IODE in subframe 3
        uint32_t iodcLSB = getBits(pframe->gpsSatSubframes[0].words, GPSL1CA_BIT(211), 8);
        uint32_t iode2 = getBits(pframe->gpsSatSubframes[1].words, GPSL1CA_BIT(61), 8);
        uint32_t iode3 = getBits(pframe->gpsSatSubframes[2].words, GPSL1CA_BIT(271), 8);
        if ((iodcLSB == iode2) && (iodcLSB == iode3)) {
            //all ephemerides have been already received; extract and store them into the RINEX object
            logMsg += LOG_MSG_IOD;
            plog->fine(logMsg);
            extractGPSL1CAEphemeris(satNum - 1, bom);
            tTag = scaleGPSEphemeris(bom, bo);
            rinex.saveNavData('G', satNum, bo, tTag);
            //clear satellite frame storage
            pframe->hasData = false;
            for (int i = 0; i < GPS_MAXSUBFRS; ++i) pframe->gpsSatSubframes[i].hasData = false;
        } else plog->fine(logMsg + " and IODs different.");
    } else plog->finer(logMsg);
    return true;
}

/**collectGPSL1CACorrections gets from GPS L1 C/A navigation raw data message the ionospheric, clock and leap corrections data
 * needed to generate some RINEX navigation files header records.
 * Satellite navigation messages are temporary stored until subframes 1 and 4 of a given satellite and epoch have been received.
 * When completed, corrections are extracted from subframes, scaled to working units, and stored into header records of the RinexData object.
 *
 * @param rinex	the class instance where data are stored
 * @param msgType the true type of the message
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectGPSL1CACorrections(RinexData &rinex, int msgType) {
    char constId;   //the constellation identifier this satellite belongs
    int satNum;		//the satellite number this navigation message belongssatt
    int sfrmNum;    //navigation message subframe number
    int pageNum;    //navigation message page number
    int bom[BO_LINSTOTAL][BO_MAXCOLS];		//a RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
    double bo[BO_LINSTOTAL][BO_MAXCOLS];	//the RINEX broadcats orbit arrangement for satellite ephemeris
    double tTag;		//the time tag for ephemeris data
    string logMsg = getMsgDescription(msgType);
    //read MT_SATNAV_GPS_L1_CA message data
    if (!readGPSL1CANavMsg(constId, satNum, sfrmNum, pageNum, logMsg)) return false;
    GPSFrameData* pframe = &gpsSatFrame[satNum - 1];
    //check if corrections have been received, that is, subframes 1 and 4 have data
    if (pframe->hasData && pframe->gpsSatSubframes[0].hasData && pframe->gpsSatSubframes[3].hasData) {
        logMsg += LOG_MSG_CORR;
        extractGPSL1CAEphemeris(satNum - 1, bom);
        tTag = scaleGPSEphemeris(bom, bo);
        rinex.setHdLnData(rinex.IONC, rinex.IONC_GPSA, bo[BO_LIN_IONOA], bom[BO_LIN_TIMEG][2], satNum);
        rinex.setHdLnData(rinex.IONC, rinex.IONC_GPSB, bo[BO_LIN_IONOB], bom[BO_LIN_TIMEG][2], satNum);
        rinex.setHdLnData(rinex.TIMC, rinex.TIMC_GPUT, bo[BO_LIN_TIMEU], 0, satNum);
        rinex.setHdLnData(rinex.LEAP, (int) bo[BO_LIN_LEAPS][0], (int) bo[BO_LIN_LEAPS][1], (int) bo[BO_LIN_LEAPS][2], (int) bo[BO_LIN_LEAPS][3], 'G');
        logMsg += "IONA&B TIMEG TIMEU LEAPS";
        plog->fine(logMsg);
        //clear satellite frame storage
        pframe->hasData = false;
        for (int i = 0; i < GPS_MAXSUBFRS; ++i) pframe->gpsSatSubframes[i].hasData = false;
    } else plog->finer(logMsg);
    return true;
}

/**readGPSL1CANavMsg read a GPS L1 C/A navigation message data from the navigation raw data file.
 * Data are stored in gpsSatFrame of the corresponding satNum.
 * For GPS L1 C/A each subframe of the navigation message contains 10 30-bit words.
 * Each word (30 bits) should be fit into the last 30 bits in a 4-byte word (skip B31 and B32), with MSB first,
 * for a total of 40 bytes, covering a time period of 6, 6, and 0.6 seconds, respectively.
 * Only have interest subframes: 1,2,3 & page 18 of subframe 4 (pgID = 56 in GPS ICD Table 20-V)
 *
 * @param constId constellation identification (G, R, E, ...)
 * @param satNum satellite number in the constellation
 * @param sfrmNum subframe number
 * @param pageNum page number
 * @param logMsg is the loggin message
 * @return true if data succesful read, false otherwise
 */
bool GNSSdataFromGRD::readGPSL1CANavMsg(char &constId, int &satNum, int &sfrmNum, int &pageNum, string &logMsg) {
    int status;     //the status of the navigation message:0=UNKNOWN, 1=PARITY PASSED, 2=REBUILT
    int msgSize;    //the message size in bytes
    unsigned int navMsg[GPS_L1_CA_MSGSIZE]; //to store GPS message bytes from receiver
    GPSSubframeData *psubframe;
    GPSFrameData *pframe;
    try {
        //read MT_SATNAV_GPS_L1_CA message data
        if (fscanf(grdFile, "%d;%c%d;%d;%d;%d", &status, &constId, &satNum, &sfrmNum, &pageNum, &msgSize) != 6
            || status < 1
            || constId != 'G'
            || satNum < GPS_MINPRN
            || satNum > GPS_MAXPRN
            || msgSize != GPS_L1_CA_MSGSIZE) {
            plog->warning(logMsg + LOG_MSG_INMP + LOG_MSG_OSIZ);
            return false;
        }
        logMsg += " sat:" + to_string(satNum) + " subfr:" + to_string(sfrmNum) + " pg:" + to_string(pageNum);
        for (int i = 0; i < GPS_L1_CA_MSGSIZE; ++i) {
            if (fscanf(grdFile,";%X", navMsg+i) != 1) {
                plog->warning(logMsg + LOG_MSG_ERRO + LOG_MSG_INMP);
                return false;
            }
        }
        if ((sfrmNum < 1 || sfrmNum > GPS_MAXSUBFRS) || (sfrmNum == 4 && pageNum != 18)) {
            plog->finer(logMsg + LOG_MSG_NAVIG);
            return false;
        }
        pframe = &gpsSatFrame[satNum - 1];
        psubframe = &pframe->gpsSatSubframes[sfrmNum - 1];
        //pack message bytes into the ten words with navigation data
        for (int i = 0, n=0; i < GPS_SUBFRWORDS; ++i, n+=4) {
            psubframe->words[i] = navMsg[n]<<24 | navMsg[n+1]<<16 | navMsg[n+2]<<8 | navMsg[n+3];
        }
        psubframe->hasData = true;
        pframe->hasData = true;
        logMsg += LOG_MSG_SFR;
        /*TODO to analyze including the code for checking satellite health
        //check for SVhealth
        uint32_t svHealth = (navMsq[4]>>10) & 0x3F;
        if ((svHealth & 0x20) != 0) {
            sprintf(errorBuff, "Wrong SV %2u health <%02hx>", sv, svHealth);
            plog->info(string(errorBuff));
            return false;
        }
        */
    } catch (int error) {
        plog->severe(logMsg + LOG_MSG_ERRO + to_string((long long) error));
        return false;
    }
    return true;
}

/**extractGPSL1CAEphemeris extract satellite ephemeris from the stored navigation frames transmitted for a given GPS satellite.
 * <p>The navigation message data of interest here have been stored in GPS frame (gpsSatFrame) data words (without parity).
 * Ephemeris are extracted from data words and stored into a RINEX Broadcast Orbit lines like arrangement.
 * <p>This method stores the MANTISSA of each satellite ephemeris into a broadcast orbit array, without applying any scale factor.
 * <p>In addition to the 8 lines x 4 ephemeris per satellite needed for RINEX, they are extracted data for iono and time corrections,
 * and the UTC leap seconds available in the navigation message and needed for the header of the file. They are stored in the
 * rows BO_LIN_IONOA, BO_LIN_IONOB, BO_LIN_TIMEx, and BO_LIN_LEAP respectively.
 *
 * @param satIdx the satellite index in the gpsSatFrame
 * @param bom an array of broadcats orbit data containing the mantissa of each satellite ephemeris
 */
void GNSSdataFromGRD::extractGPSL1CAEphemeris(int satIdx, int (&bom)[BO_LINSTOTAL][BO_MAXCOLS]) {
    uint32_t *psubfr1data = gpsSatFrame[satIdx].gpsSatSubframes[0].words;
    uint32_t *psubfr2data = gpsSatFrame[satIdx].gpsSatSubframes[1].words;
    uint32_t *psubfr3data = gpsSatFrame[satIdx].gpsSatSubframes[2].words;
    uint32_t *psubfr4data = gpsSatFrame[satIdx].gpsSatSubframes[3].words;
    for (int i = 0; i < BO_LINSTOTAL; ++i) {
        for (int j = 0; j < BO_MAXCOLS; ++j) {
            bom[i][j] = 0;
        }
    }
    //broadcast line 0
    bom[0][0] = getBits(psubfr1data, GPSL1CA_BIT(219), 16);	                    //T0C
    bom[0][1] = getTwosComplement(getBits(psubfr1data, GPSL1CA_BIT(271), 22), 22);	//Af0
    bom[0][2] = getTwosComplement(getBits(psubfr1data, GPSL1CA_BIT(249), 16), 16);	//Af1
    bom[0][3] = getTwosComplement(getBits(psubfr1data, GPSL1CA_BIT(241), 8), 8);	//Af2
    //broadcast line 1
    bom[1][0] = getBits(psubfr2data, GPSL1CA_BIT(61), 8);	                        //IODE
    bom[1][1] = getTwosComplement(getBits(psubfr2data, GPSL1CA_BIT(69), 16), 16);	//Crs
    bom[1][2] = getTwosComplement(getBits(psubfr2data, GPSL1CA_BIT(91), 16), 16);	//Delta n
    bom[1][3] = (getBits(psubfr2data, GPSL1CA_BIT(107), 8) << 24) | getBits(psubfr2data, GPSL1CA_BIT(121), 24);//M0
    //broadcast line 2
    bom[2][0] = getTwosComplement(getBits(psubfr2data, GPSL1CA_BIT(151), 16), 16);			                            //Cuc
    bom[2][1] = (getBits(psubfr2data, GPSL1CA_BIT(167), 8) << 24) | getBits(psubfr2data, GPSL1CA_BIT(181), 24);	//e
    bom[2][2] = getTwosComplement(getBits(psubfr2data, GPSL1CA_BIT(211), 16), 16);		                                //Cus
    bom[2][3] = (getBits(psubfr2data, GPSL1CA_BIT(227), 8) << 24) | getBits(psubfr2data, GPSL1CA_BIT(241), 24);	//sqrt(A)
    //broadcast line 3
    bom[3][0] = getBits(psubfr2data, GPSL1CA_BIT(271), 16);	                                                        //Toe
    bom[3][1] = getTwosComplement(getBits(psubfr3data, GPSL1CA_BIT(61), 16), 16);						                //Cic
    bom[3][2] = (getBits(psubfr3data, GPSL1CA_BIT(77), 8) << 24) | getBits(psubfr3data, GPSL1CA_BIT(91), 24); 	//OMEGA0
    bom[3][3] = getTwosComplement(getBits(psubfr3data, GPSL1CA_BIT(121), 16), 16);		                				//CIS
    //broadcast line 4
    bom[4][0] = (getBits(psubfr3data, GPSL1CA_BIT(137), 8) << 24) | getBits(psubfr3data, GPSL1CA_BIT(151), 24);	//i0
    bom[4][1] = getTwosComplement(getBits(psubfr3data, GPSL1CA_BIT(181), 16), 16);										//Crc
    bom[4][2] = (getBits(psubfr3data, GPSL1CA_BIT(197), 8) << 24) | getBits(psubfr3data, GPSL1CA_BIT(211), 24);	//w (omega)
    bom[4][3] = getTwosComplement(getBits(psubfr3data, GPSL1CA_BIT(241), 24), 24);                                     //w dot
    //broadcast line 5
    bom[5][0] = getTwosComplement(getBits(psubfr3data, GPSL1CA_BIT(279), 14), 14);	//IDOT
    bom[5][1] = getBits(psubfr1data, GPSL1CA_BIT(71), 2);		            		//Codes on L2
    bom[5][2] = getBits(psubfr1data, GPSL1CA_BIT(61), 10) + nGPSrollOver * 1024; 	//GPS week#
    bom[5][3] = getBits(psubfr1data, GPSL1CA_BIT(91), 1);				            //L2P data flag
    //broadcast line 6
    bom[6][0] = getBits(psubfr1data, GPSL1CA_BIT(73), 4);		//URA index
    bom[6][1] = getBits(psubfr1data, GPSL1CA_BIT(77), 6);      	//SV health
    bom[6][2] = getTwosComplement(getBits(psubfr1data, GPSL1CA_BIT(197), 8), 8);                                //TGD
    bom[6][3] = (getBits(psubfr1data, GPSL1CA_BIT(83), 2) << 8) | getBits(psubfr1data, GPSL1CA_BIT(211), 8);	//IODC
    //broadcast line 7
    bom[7][0] = getBits(psubfr1data, GPSL1CA_BIT(31), 17) *6 *100; //Transmission time of message:
                // the 17 MSB of the Zcount in HOW converted to sec and scaled by 100
    bom[7][1] = getBits(psubfr2data, GPSL1CA_BIT(287), 1);			//Fit interval flag
    bom[BO_LIN_IONOA][0] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(69), 8), 8);    //alfa0
    bom[BO_LIN_IONOA][1] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(77), 8), 8);    //alfa1
    bom[BO_LIN_IONOA][2] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(91), 8), 8);    //alfa2
    bom[BO_LIN_IONOA][3] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(99), 8), 8);    //alfa3
    bom[BO_LIN_IONOB][0] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(107), 8), 8);   //beta0
    bom[BO_LIN_IONOB][1] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(121), 8), 8);   //beta1
    bom[BO_LIN_IONOB][2] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(129), 8), 8);   //beta2
    bom[BO_LIN_IONOB][3] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(137), 8), 8);   //beta3
    bom[BO_LIN_TIMEU][0] = (getBits(psubfr4data, GPSL1CA_BIT(181), 24) << 8) | getBits(psubfr4data, GPSL1CA_BIT(211), 8);    //Time correction A0
    bom[BO_LIN_TIMEU][1] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(151), 24), 24);    //Time correction A1
    bom[BO_LIN_TIMEU][2] = getBits(psubfr4data, GPSL1CA_BIT(219), 8);       //t0t, ref time for for UTC data
    bom[BO_LIN_TIMEU][3] = getBits(psubfr4data, GPSL1CA_BIT(227), 8);       //WNt, UTC ref week number
    bom[BO_LIN_TIMEU][3] |= bom[5][2] & (~MASK8b);       //put WNt as a continuous week number
    bom[BO_LIN_TIMEG][2] = getBits(psubfr4data, GPSL1CA_BIT(31), 17) * 6; //Transmission time of message: the 17 MSB of the Zcount in HOw
    bom[BO_LIN_TIMEG][3] = bom[5][2];      //the week number when message was transmitted
    bom[BO_LIN_LEAPS][0] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(241), 8), 8);    //delta tLS (leap seconds)
    bom[BO_LIN_LEAPS][1] = getTwosComplement(getBits(psubfr4data, GPSL1CA_BIT(271), 8), 8);    //delta tLSF
    bom[BO_LIN_LEAPS][2] = getBits(psubfr4data, GPSL1CA_BIT(249), 8);    //WN_LSF
    bom[BO_LIN_LEAPS][2] |= bom[5][2] & (~MASK8b);       //put WN_LSF as a continuous week number
    bom[BO_LIN_LEAPS][3] = getBits(psubfr4data, GPSL1CA_BIT(257), 8);    //DN_LSF
}

/**scaleGPSEphemeris apply GPS scale factors to satellite ephemeris mantissas to obtain true satellite ephemeris and store them
 * in RINEX broadcast orbit like arrangements.
 * The given navigation data are the mantissa parameters as transmitted in the GPS navigation message, that shall be
 * converted to the true ephemeris values applying the corresponding scale factor.
 * It is obtained a time tag from the week number and time of week inside the satellite ephemeris. Its value are the seconds
 * transcurred from the GPS epoch.
 *
 * @param bom the mantissas of orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @param bo the orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @return the time tag of the satellite ephemeris as seconds from the GPS epoch
 */
double GNSSdataFromGRD::scaleGPSEphemeris(int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], double (&bo)[BO_LINSTOTAL][BO_MAXCOLS]) {
    double aDouble;
    int iodc = bom[6][3];
    for(int i=0; i<BO_LINSTOTAL; i++) {
        for (int j = 0; j < BO_MAXCOLS; j++) {
            bo[i][j] = bom[i][j] * GPS_SCALEFACTOR[i][j];
        }
    }
    //recompute the ones being 32 bits twos complement signed integers
    bo[2][1] = ((unsigned int) bom[2][1]) * GPS_SCALEFACTOR[2][1];  //e
    bo[2][3] = ((unsigned int) bom[2][3]) * GPS_SCALEFACTOR[2][3];  //sqrt(A)
    bo[BO_LIN_TIMEU][0] = ((unsigned int) bom[BO_LIN_TIMEU][0]) * GPS_SCALEFACTOR[BO_LIN_TIMEU][0]; //A0
    //fit interval
    if (bom[7][1] == 0) aDouble = 4.0;
    else if (iodc>=240 && iodc<=247) aDouble = 8.0;
    else if (iodc>=248 && iodc<=255) aDouble = 14.0;
    else if (iodc==496) aDouble = 14.0;
    else if (iodc>=497 && iodc<=503) aDouble = 26.0;
    else if (iodc>=1021 && iodc<=1023) aDouble = 26.0;
    else aDouble = 6.0;
    bo[7][1] = aDouble;
    //compute User Range Accuracy value
    bo[6][0] = bom[6][0] < 16? GPS_URA[bom[6][0]] : GPS_URA[15];
    return getInstantGNSStime(
            bom[5][2],	//GPS week# whitout roll over
            bo[0][0]);  //T0c
}
#undef GPSL1CA_BIT

//methods for GLONASS L1 CA message processing
//A macro to get bit position in a stream from the bit position (BITNUMBER) in the message string stated in the GLONASS ICD
//Each GLONASS 85 bits string is stored in an array of 3 uint32_t (the stream)
#define GLOL1CA_BIT(BITNUMBER) (85 - BITNUMBER)

/**collectGLOL1CAEphemeris gets GLONASS navigation data from a raw message and store them to generate RINEX navigation files.
 * Satellite navigation messages are temporary stored until all needed swtrings for the current satellite and epoch have been received.
 * When completed, satellite ephemeris are extracted from strings, scaled to working units, and stored into "broadcast orbit lines"
 * of the RinexData object.
 * <p>For Glonass L1 C/A, each string contains 85 data bits, including the checksum.
 * <p>These bits should be fit into 11 bytes, with MSB first (skip B86-B88), covering a time period of 2 seconds
 * <p>The satellite number given by the satellite can be:
 * <p>- OSN case: it is in the range 1-24 and is the slot number
 * <p>- FCN case: it is in the range 96-106 obtainded from FCN (-7 to +6) + 100. The slot number is in string 4 bits 15-11

 * @param rinex	the class instance where data are stored
 * @param msgType the true type of the message
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectGLOL1CAEphemeris(RinexData &rinex, int msgType) {
    char constId;   //the constellation identifier this satellite belongs
    int satNum, satIdx;		//the satellite number this navigation message belongssatt
    int strNum;     //navigation message string number
    int frmNum;      //navigation message frame number
    //needed for RINEX
    int bom[BO_LINSTOTAL][BO_MAXCOLS];			//the RINEX broadcats orbit like arrangement for satellite ephemeris mantissa (as extracted from nav message)
    double bo[BO_LINSTOTAL][BO_MAXCOLS];		//the RINEX broadcats orbit arrangement for satellite ephemeris (after applying scale factors)
    uint32_t wd[GLO_STRWORDS]; //a place to store the 84 bits of a GLONASS string
    double tTag;    //the time tag for ephemeris data
    int sltnum;     //the GLONASS slot number extracted from navigation message
    string logmsg = getMsgDescription(msgType);			//a place to build log messages
    //read MT_SATNAV_GLONASS_L1_CA message data
    if (!readGLOL1CANavMsg(constId, satNum, satIdx, strNum, frmNum, logmsg)) return false;
    //check if all ephemerides have been received (all strings received)
    GLOFrameData* pFrame = &gloSatFrame[satIdx];
    bool allRec = pFrame->frmNum != 0;
    for (int i = 0; allRec && i < GLO_MAXSTRS; ++i) allRec = allRec && pFrame->gloSatStrings[i].hasData;
    if (allRec) {
        //all ephemerides have been already received; extract and store them into the RINEX object
        extractGLOL1CAEphemeris(satIdx, bom, sltnum);
        logmsg += " Frame completed";
        if ((sltnum < GLO_MINOSN) || (sltnum > GLO_MAXOSN)) {
            logmsg + ", but out of range";
        } else {
            tTag = scaleGLOEphemeris(bom, bo);
            rinex.saveNavData('R', sltnum, bo, tTag);
        }
        plog->fine(logmsg);
        //clear satellite string storage
        pFrame->frmNum = 0;
        for (int i = 0; i < GLO_MAXSTRS; ++i) pFrame->gloSatStrings[i].hasData = false;
    } else plog->finer(logmsg);
    return true;
}

/**collectGLOL1CACorrections gets from GLONASS L1 C/A navigation messages contained in the navigation raw data file the clock and leap
 * corrections data needed to generate some RINEX navigation files header records.
 * Satellite navigation messages are temporary stored until all needed strings (4 & 5) of a given satellite and epoch have been received.
 * When completed, corrections are extracted from subframes, scaled to working units, and stored into header records of the RinexData object.
 *
 * @param rinex	the class instance where data are stored
 * @param msgType the true type of the message
 * @return true if data properly extracted, false otherwise
 */
bool GNSSdataFromGRD::collectGLOL1CACorrections(RinexData &rinex, int msgType) {
    char constId;       //the constellation identifier this satellite belongs
    int satNum;     //the satellite number: 1-24 or 93-106
    int satIdx; 	//the satellite number after being normalized: 0-23 for OSN or 24- for FCN
    int satOSN;     //the Orbital Slot Number
    int strNum;     //navigation message string number
    int frmNum;         //navigation message frame number
    int bom[BO_LINSTOTAL][BO_MAXCOLS];		//a RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
    double bo[BO_LINSTOTAL][BO_MAXCOLS];	//the RINEX broadcats orbit arrangement for satellite ephemeris
    uint32_t wd[GLO_STRWORDS]; //a place to store the 84 bits of a GLONASS string
    double tTag;		//the time tag for ephemeris data
    string logMsg = getMsgDescription(msgType);
    //read MT_SATNAV_GLO_L1_CA message data
    if (!readGLOL1CANavMsg(constId, satNum, satIdx, strNum, frmNum, logMsg)) return false;
    //check if all strings (4 & 5) with corrections have been received
    GLOFrameData* pFrame = &gloSatFrame[satIdx];
    if (pFrame->frmNum != 0
        && pFrame->gloSatStrings[3].hasData
        && pFrame->gloSatStrings[4].hasData) {
        //extract and store corrections
        extractGLOL1CAEphemeris(satIdx, bom, satOSN);
        logMsg += " Corrections completed";
        if ((satOSN < GLO_MINOSN) || (satOSN > GLO_MAXOSN)) {
            logMsg += ", but OSN out of range";
        } else {
            //set values in table OSN-FCN
            GLONASSosnfcn* pOSN_FCN = &glonassOSN_FCN[satIdx];
            pOSN_FCN->osn = satOSN;
            if (satIdx > GLO_MAXOSN && !pOSN_FCN->fcnSet) {
                //set FCN value extracted from satNum
                pOSN_FCN->fcn = satIdx + GLO_FCN2OSN - 100;
                pOSN_FCN->fcnSet = true;
            }
            //set time corrections
            tTag = scaleGLOEphemeris(bom, bo);
            rinex.setHdLnData(rinex.TIMC, rinex.TIMC_GLUT, bo[BO_LIN_TIMEU], 0, satOSN);
            rinex.setHdLnData(rinex.TIMC, rinex.TIMC_GLGP, bo[BO_LIN_TIMEG], 0, satOSN);
            logMsg += " TIMEU TIMEG";
        }
        plog->fine(logMsg);
        //clear satellite string storage
        pFrame->frmNum = 0;
        for (int i = 0; i < GLO_MAXSTRS; ++i) pFrame->gloSatStrings[i].hasData = false;
    } else plog->finer(logMsg);
    return true;
}

/**readGLOL1CANavMsg read a GLONASS navigation message from the navigation raw data file and save data read.
 * Data are stored in the gloSatFrame and gloSatStrings of the corresponding satNum and string.
 * In addition it updates the Glonass OSN_FCN table. See explanation for GLONASSosnfcn struct in GNSSdataFromGRD.h
 * and GLONASS ICD for details.
 *
 * @param constId constellation identification (G, R, E, ...)
 * @param satNum satellite number in the constellation
 * @param satIdx the index of the satellite number
 * @param strnum string number
 * @param frame frame number
 * @param wd a place to store the bytes of the navigation message
 * @param logMsg is the loggin message
 * @return true if data succesful read, false otherwise
 */
bool GNSSdataFromGRD::readGLOL1CANavMsg(char &constId, int &satNum, int &satIdx, int &strNum, int &frmNum, string &logMsg) {
    int status;     //the status of the navigation message:0=UNKNOWN, 1=PARITY PASSED, 2=REBUILT
    int msgSize;    //the message size in bytes
    int nA;         //the slot number from almanac
    GLONASSfreq* ptf;   //pointers to speed processing
    GLONASSosnfcn* pto;
    GLOFrameData* ptFrm;
    GLOStrData* pString;
    unsigned int navMsg[GLO_STRWORDS * 4];
    uint32_t wd[GLO_STRWORDS]; //a place to store the 84 bits of a GLONASS string
    try {
        //read MT_SATNAV_GLONASS_L1_CA message data
        if (fscanf(grdFile, "%d;%c%d;%d;%d;%d", &status, &constId, &satNum, &strNum, &frmNum, &msgSize) != 6
            || msgSize != GLO_L1_CA_MSGSIZE
            || status < 1) {
            plog->warning(logMsg + LOG_MSG_INMP + LOG_MSG_OSIZ);
            return false;
        }
        logMsg += " sat:" + to_string(satNum) + " str:" + to_string(strNum) + " frm:" + to_string(frmNum);
        if (frmNum < 1 || frmNum > 5) {
            plog->finer(logMsg + " Frame ignored");
            return false;
        }
        if ((satIdx = gloSatIdx(satNum)) == GLO_MAXSATELLITES) {
            plog->warning(logMsg + " GLO sat number not OSN or FCN");
            return false;
        }
        for (int i = 0; i < GLO_L1_CA_MSGSIZE; ++i) {
            if (fscanf(grdFile,";%X", navMsg+i) != 1) {
                plog->warning(logMsg + LOG_MSG_INMP);
                return false;
            }
        }
        //set OSN or FCN values directly from satNum if possible
        GLONASSosnfcn* pOSN_FCN = &glonassOSN_FCN[satIdx];
        if (satIdx < GLO_MAXOSN) pOSN_FCN->osn = satNum;
        else if (!pOSN_FCN->fcnSet) {
            //set FCN value extracted from satNum
            pOSN_FCN->fcn = satNum - 100;
            pOSN_FCN->fcnSet = true;
        }
        //pack message bytes into words with navigation data
        for (int i = 0; i < GLO_STRWORDS; ++i) {
            wd[i] = navMsg[i*4]<<24 | navMsg[i*4+1]<<16 | navMsg[i*4+2]<<8 | navMsg[i*4+3];
        }
        switch (strNum) {
            case 4:
                //string 4 includes de OSN (n in the ICD). Save it in the glonassOSN_FCN table
                if (glonassOSN_FCN[satIdx].osn == 0) {
                    glonassOSN_FCN[satIdx].osn = getBits(wd, GLOL1CA_BIT(15), 5);
                    logMsg += " Is OSN " + to_string(glonassOSN_FCN[satIdx].osn);
                }
            case 1:
            case 2:
            case 3:
            case 5:
                //save immediate information with ephemeris (strings 1 to 5)
                ptFrm = &gloSatFrame[satIdx];
                if (ptFrm->frmNum != frmNum) {
                    //frame number has chaged: discard former data saved
                    ptFrm->frmNum = frmNum;
                    for (int i = 0; i < GLO_MAXSTRS; ++i) ptFrm->gloSatStrings[i].hasData = false;
                }
                pString = &gloSatFrame[satIdx].gloSatStrings[strNum - 1];
                for (int i = 0; i < GLO_STRWORDS; ++i) pString->words[i] = wd[i];
                pString->hasData = true;
                logMsg += " String saved in " + to_string(satIdx);
                return true;
            case 6:
            case 8:
            case 10:
            case 12:
            case 14:
                //get from almanac data the Orbital Slot Number (nA in bits 77-73, see GLONASS ICD for details)
                //and save it in the nAhNA table, and also the slot - frame where the FCN value should came
                nA = getBits(wd, GLOL1CA_BIT(77), 5);
                if (nA >= GLO_MINOSN && nA <= GLO_MAXOSN) {
                    logMsg += " Almanac OSN " + to_string(nA);
                    nA--;   //converted to an index to the table
                    //set the string & frame number where FCN will come
                    nAhnA[nA].strFhnA = strNum + 1;
                    nAhnA[nA].frmFhnA = frmNum;
                    return true;
                }
                plog->warning(logMsg + " Bad OSN " + to_string(nA));
                break;
            case 7:
            case 9:
            case 11:
            case 13:
            case 15:
                //check if this is the expected string & frame number including HnA
                for (nA = 0; nA < GLO_MAXOSN; nA++) {
                    ptf = &nAhnA[nA];
                    if ((ptf->strFhnA == strNum) && (ptf->frmFhnA == frmNum)) {
                        pto = &glonassOSN_FCN[nA];
                        if (!pto->fcnSet) {
                            pto->osn = nA + 1;
                            pto->fcn = getBits(wd, GLOL1CA_BIT(14), 5); //HnA (carrier frequency number) in almanac: bits 14-10
                            if (pto->fcn > 24) pto->fcn -= 32;  //from table 4.10 of the ICD
                            pto->fcnSet = true;
                            logMsg += " Almanac FCN " + to_string(pto->fcn) + " for OSN " + to_string(pto->osn);
                        }
                        return true;
                    }
                }
                plog->fine(logMsg + " Unexpected almanac string");
                break;
            default:
                plog->fine(logMsg + LOG_MSG_NAVIG);
                break;
        }
    } catch (int error) {
        plog->severe(logMsg + LOG_MSG_ERRO + to_string((long long) error));
    }
    return false;
}

/**extractGLOL1CAEphemeris extract satellite number, time tag, and ephemeris from the given navigation message string transmitted by GLONASS satellites.
 * <p>The navigation message data frames of interest here have been stored in GLONASS strings for each satellite.
 * Ephemeris are extracted from strings and stored into a RINEX Broadcast Orbit lines like arrangement.
 * <p>This method stores the MANTISSA of each satellite ephemeris into a broadcast orbit array, without applying any scale factor.
 * <p>In addition to the 4 lines x 4 ephemeris per satellite needed for RINEX, for the header of the file they are extracted
 * the time corrections and the UTC leap seconds available in the navigation message. They are stored in the
 * rows BO_LIN_TIMEx, and BO_LIN_LEAP respectively.
 *
 * @param satIdx the satellite index
 * @param bom an array of broadcats orbit data containing the mantissa of each satellite ephemeris
 * @param slt the slot number extracted from navigation message
 */
void GNSSdataFromGRD::extractGLOL1CAEphemeris(int satIdx, int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], int& slt) {
    uint32_t *pstring1 = gloSatFrame[satIdx].gloSatStrings[0].words;
    uint32_t *pstring2 = gloSatFrame[satIdx].gloSatStrings[1].words;
    uint32_t *pstring3 = gloSatFrame[satIdx].gloSatStrings[2].words;
    uint32_t *pstring4 = gloSatFrame[satIdx].gloSatStrings[3].words;
    uint32_t *pstring5 = gloSatFrame[satIdx].gloSatStrings[4].words;
    for (int i = 0; i < BO_LINSTOTAL; ++i) {
        for (int j = 0; j < BO_MAXCOLS; ++j) {
            bom[i][j] = 0;
        }
    }
    //TODO verify need or not for string 5
    /*  not used in this version
    slt = getBits(pstring4, GLOL1CA_BIT(15), 5);	//slot number (n) in string 4, bits 15-11
    */
    slt = glonassOSN_FCN[satIdx].osn;
    int n4 = getBits(pstring5, GLOL1CA_BIT(36), 5);	//four-year interval number N4: bits 36-32 string 5
    int nt = getBits(pstring4, GLOL1CA_BIT(26), 11);	//day number NT: bits 26-16 string 4
    int tb = getBits(pstring2, GLOL1CA_BIT(76), 7); //time interval index tb: bits 76-70 string 2
    tb *= 15*60;	//convert index to secs
    /*  not used in this version
    uint32_t tk =  getBits(pstring1, GLOL1CA_BIT(76), 12);	//Message frame time: bits 76-65 string 1
    int tkSec = (tk >> 7) *60*60;		//hours in tk to secs.
    tkSec += ((tk >> 1) & 0x3F) * 60;	//add min. in tk to secs.
    tkSec += (tk & 0x01) == 0? 0: 30;	//add sec. interval to secs.
    */
    //convert frame time (in GLONASS time) to an UTC instant (use GPS ephemeris for convenience)
    double tTag = getInstantGPSdate(1996 + (n4-1)*4, 1, nt, 0, 0, (float) tb) - 3*60*60;
    bom[0][0] = (int) tTag;													//Toc
    bom[0][1] = -getSigned(getBits(pstring4, GLOL1CA_BIT(80), 22), 22);	//Clock bias TauN: bits 80-59 string 4
    bom[0][2] = getSigned(getBits(pstring3, GLOL1CA_BIT(79), 11), 11);	//Relative frequency bias GammaN: bits 79-69 string 3
    bom[0][3] = ((int) getTowGNSSinstant(tTag) + 518400) % 604800;		//seconds from UTC week start (mon 00:00). Note that GNSS week starts sun 00:00.
    bom[1][0] = getSigned(getBits(pstring1, GLOL1CA_BIT(35), 27), 27);	//Satellite position, X: bits 35-9 string 1
    bom[1][1] = getSigned(getBits(pstring1, GLOL1CA_BIT(64), 24), 24);	//Satellite velocity, X: bits 64-41 string 1
    bom[1][2] = getSigned(getBits(pstring1, GLOL1CA_BIT(40), 5), 5);	//Satellite acceleration, X: bits 40-36 string 1
    bom[1][3] = getBits(pstring2, GLOL1CA_BIT(80), 3);					//Satellite health Bn: bits 80-78 string 2
    bom[2][0] = getSigned(getBits(pstring2, GLOL1CA_BIT(35), 27), 27);	//Satellite position, Y: bits 35-9 string 2
    bom[2][1] = getSigned(getBits(pstring2, GLOL1CA_BIT(64), 24), 24);	//Satellite velocity, Y: bits 64-41 string 2
    bom[2][2] = getSigned(getBits(pstring2, GLOL1CA_BIT(40), 5), 5);	//Satellite acceleration, Y: bits 40-36 string 2
    bom[2][3] = glonassOSN_FCN[satIdx].fcn;								//Frequency number (-7 ... +6)
    bom[3][0] = getSigned(getBits(pstring3, GLOL1CA_BIT(35), 27), 27);	//Satellite position, Z: bits 35-9 string 3
    bom[3][1] = getSigned(getBits(pstring3, GLOL1CA_BIT(64), 24), 24);	//Satellite velocity, Z: bits 64-41 string 3
    bom[3][2] = getSigned(getBits(pstring3, GLOL1CA_BIT(40), 5), 5);	//Satellite acceleration, Z: bits 40-36 string 3
    bom[3][3] = getBits(pstring4, GLOL1CA_BIT(53), 5);			//Age of oper. information (days) (En): bits 53-49 string 4
    //get time corrections data from string 5
    bom[BO_LIN_TIMEU][0] = getBits(pstring5, GLOL1CA_BIT(69), 32);   //TauC
    bom[BO_LIN_TIMEG][0] = getBits(pstring5, GLOL1CA_BIT(31), 22);   //TauGPS
}

/**scaleGLOEphemeris apply scale factors to satellite ephemeris mantissas to obtain true satellite ephemeris and store them
 * in RINEX broadcast orbit like arrangements.
 * The given navigation data are the mantissa parameters as transmitted in the GLONASS navigation message, that shall be
 * converted to the true ephemeris values applying the corresponding scale factor.
 * It is obtained a time tag from calendar data in the nav message. Its value are the UTC seconds transcurred from the GPS epoch.
 *
 * @param bom the mantissas of orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @param bo the orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @return the time tag of the satellite ephemeris as UTC seconds from the GPS epoch
 */
double GNSSdataFromGRD::scaleGLOEphemeris(int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], double (&bo)[BO_LINSTOTAL][BO_MAXCOLS]) {
    for(int i=0; i<BO_LINSTOTAL; i++) {
        for(int j=0; j<BO_MAXCOLS; j++) {
            bo[i][j] =  bom[i][j] * GLO_SCALEFACTOR[i][j];
        }
    }
    //recompute the ones being 32 bits twos complement signed integers
    bo[BO_LIN_TIMEU][0] = ((unsigned int) bom[BO_LIN_TIMEU][0]) * GLO_SCALEFACTOR[BO_LIN_TIMEU][0];  //TauC
    return (double) bom[0][0];
}

/**gloSatIdx obtains the satellite index from the given OSN or FCN.
 * If the given satellite number is the OSN range index will be in the range 0 to GLO_MAXOSN-1
 * If the satellite number is FCN, index will be in the range GLO_MAXOSN to GLO_MAXFCN - GLO_FCN2OSN
 *
 * @param stn the satellite number (may be OSN or FCN)
 * @return the satellite index or 0 if the given stn is not OSN or FCN
 */
int GNSSdataFromGRD::gloSatIdx(int stn) {
    if (stn >= GLO_MINOSN && stn <= GLO_MAXOSN) return (stn - 1);  //normal OSN case, index in range 0 - 23
    if (stn >= GLO_MINFCN && stn <= GLO_MAXFCN) return (stn - GLO_FCN2OSN); //case FCN, translated to index in range 24 - 37
    return GLO_MAXSATELLITES;
}

/**
 * gloOSN obtains the GLONASS Orbital Satellite Nnumber (OSN) for the given satellite number from the data available,
 * and, if requested, set OSN and Frequency Channel Number (FCN) if data available.
 * There is a glonassOSN_FCN table where OSN and FCN values are saved for each satellite number index (satNum -1).
 * This table is constructed using data from several sources:
 *  - the own satNum is the OSN if it is in the range [1,24]
 *  - the own satNum can provide the FCN if it is in the range [93,106]
 *  - string 4 of the navigation message provides the OSN (as n, see GLONASS ICD) of the satellite
 *  - OSN is nA in the almanac strings 6, 8, 10, 12, 14) (see ICD)
 *  - FCN is hnA in the alamanac strings 7, 9, 11, 13, 15 (see ICD)
 * If the given satellite number is in the range [GLO_MINOSN, GLO_MAXOSN], it is already OSN
 * If the given satellite number is the range [GLO_MINFCN, GLO_MAXFCN] translate it to the range [GLO_MAXOSN+1, GLO_MAXSATELLITES]
 * and obtain the OSN number stored in the table for this normalized number (it shall be obtained from the navigation message).
 * If it is requested to update the OSN-FCN table, get OSN or FCN from the given satellite number and update the table.
 *
 * @param satNum the satellite number
 * @param carrFrq the carrier frequency of the L1 band signal
 * @param updTbl if true, the glonasOSN_FCN table is update,if false, it is not updated
 * @return the OSN satellite number in the range 1 to 24, or 0 if the there are not data available to obtain it
 */
int GNSSdataFromGRD::gloOSN(int satNum, char band, double carrFrq, bool updTbl) {
    double bandFrq;
    double slotFrq;
    int satIdx;
    if ((satIdx = gloSatIdx(satNum)) == GLO_MAXSATELLITES) return 0; //wrong satellite number
    GLONASSosnfcn* pt = &glonassOSN_FCN[satIdx];
    if (updTbl) {
        if (satNum <= GLO_MAXOSN) pt->osn = satNum;
        if (!pt->fcnSet) {
            //update FCN using the given data
            if (band == '2') {
                bandFrq = GLO_BAND_FRQ2;
                slotFrq = GLO_SLOT_FRQ2;
            } else {
                bandFrq = GLO_BAND_FRQ1;
                slotFrq = GLO_SLOT_FRQ1;
            }
            pt->fcn = (int) ((carrFrq - bandFrq) / slotFrq);
            pt->fcnSet = true;
            //when OSN data is available, copy this OSN-FCN entry to the lower part of the table
            satIdx = gloSatIdx(pt->osn);
            if (satIdx != GLO_MAXSATELLITES) glonassOSN_FCN[satIdx] = *pt;
        }
    }
    return pt->osn;
}
#undef GLOL1CA_BIT

//methods for GALILEO I/NAV message processing
//a macro to get bit position in a message word (0 to 127) from what is bit position stated in the Galileo ICD
#define GALIN_BIT(BITNUMBER) BITNUMBER

/**collectGALINEphemeris gets from a GALILEO I/NAV navigation data from a raw message and store them to generate RINEX navigation files.
 * Satellite navigation messages are temporary stored until all words having ephemeris (1,2,3,4 & 5) for the current satellite and epoch
 * have been received.
 * When completed, satellite ephemeris are extracted from words, scaled to working units, and stored into "broadcast orbit lines"
 * of the RinexData object.
 * For Galileo I/NAV, each page contains 2 page parts, even and odd, with a total of 2x114 = 228 bits, (sync & tail excluded)
 * that should be fit into 29 bytes, with MSB first (skip B229-B232).
 *
 * @param rinex	the class instance where data are stored
 * @param msgType the true type of the message
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectGALINEphemeris(RinexData &rinex, int msgType) {
    char constId;   //the constellation identifier this satellite belongs
    int satNum;		//the satellite number this navigation message belongssatt
    int sfrmNum;    //navigation message subframe number
    int wordNum;    //navigation message page number
    int bom[BO_LINSTOTAL][BO_MAXCOLS];	//a RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
    double bo[BO_LINSTOTAL][BO_MAXCOLS];	//the RINEX broadcats orbit arrangement for satellite ephemeris
    double tTag;		//the time tag for ephemeris data
    string logMsg = getMsgDescription(msgType);
    //read MT_SATNAV_GALIN message data
    if (!readGALINNavMsg(constId, satNum, sfrmNum, wordNum, logMsg)) return false;
    //check if all ephemerides have been received, that is:
    //-message word types 1, 2, 3, 4 & 5 have data
    //-and data in words 1 to 4 have the same IODnav (Issue Of Data)
    GALINAVFrameData *psatFrame = &galInavSatFrame[satNum - 1];
    bool allRec = psatFrame->hasData;
    for (int i = 0; allRec && i < 5; ++i) allRec = allRec && psatFrame->pageWord[i].hasData;
    if (allRec) logMsg += LOG_MSG_FRM;
    //verify IOD of data received in word types 1 to 4
    uint32_t iodNav = getBits(psatFrame->pageWord[0].data, GALIN_BIT(6), 10);
    for (int i = 1; allRec && i < 4; i++) {
        allRec = allRec && (iodNav == getBits(psatFrame->pageWord[i].data, GALIN_BIT(6), 10));
    }
    if (allRec) {
        //all ephemerides have been already received; extract and store them into the RINEX object
        logMsg += LOG_MSG_IOD;
        plog->fine(logMsg);
        extractGALINEphemeris(satNum - 1, bom);
        tTag = scaleGALEphemeris(bom, bo);
        rinex.saveNavData('E', satNum, bo, tTag);
        //clear satellite pageword storage
        psatFrame->hasData = false;
        for (int i = 0; i < GALINAV_MAXWORDS; ++i) psatFrame->pageWord[i].hasData = false;
    }
    else plog->finer(logMsg);
    return true;
}

/**collectGALINCorrections gets from a GALILEO I/NAV raw data message the ionospheric, clock and leap corrections data
 * needed to generate some RINEX navigation files header records.
 * Satellite navigation messages are temporary stored until a page type 5, 6 or 10 has been received.
 * When received, corrections are extracted from pages, scaled to working units, and stored into header records of the RinexData object.
 * For Galileo I/NAV, each page contains 2 page parts, even and odd, with a total of 2x114 = 228 bits, (sync & tail excluded)
 * that should be fit into 29 bytes, with MSB first (skip B229-B232).
 *
 * @param rinex	the class instance where data are stored
 * @param msgType the true type of the message
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectGALINCorrections(RinexData &rinex, int msgType) {
    char constId;   //the constellation identifier this satellite belongs
    int satNum;		//the satellite number this navigation message belongssatt
    int sfrmNum;    //navigation message subframe number
    int wordNum;    //navigation message page number
    int bom[BO_LINSTOTAL][BO_MAXCOLS];	//a RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
    double bo[BO_LINSTOTAL][BO_MAXCOLS];	//the RINEX broadcats orbit arrangement for satellite ephemeris
    double tTag;		//the time tag for ephemeris data
    string logMsg = getMsgDescription(msgType);
    //read MT_SATNAV_GALIN message data
    if (!readGALINNavMsg(constId, satNum, sfrmNum, wordNum, logMsg)) return false;
    //check if corrections have been received, that is page words 5, 6 or 10 have data
    GALINAVFrameData *psatFrame = &galInavSatFrame[satNum - 1];
    if (psatFrame->hasData && (psatFrame->pageWord[4].hasData || psatFrame->pageWord[5].hasData || psatFrame->pageWord[9].hasData)) {
        logMsg += LOG_MSG_CORR;
        extractGALINEphemeris(satNum - 1, bom);
        tTag = scaleGALEphemeris(bom, bo);
        if (psatFrame->pageWord[4].hasData) {   //page word type 5 includes Az and GST
            rinex.setHdLnData(rinex.IONC, rinex.IONC_GAL, bo[BO_LIN_IONOA], (int) bo[7][0], satNum);
            logMsg += "IONA";
        }
        if (psatFrame->pageWord[5].hasData) {   //page word type 6 includes GST-UTC conversion parms.
            rinex.setHdLnData(rinex.TIMC, rinex.TIMC_GAUT, bo[BO_LIN_TIMEU], 0, satNum);
            rinex.setHdLnData(rinex.LEAP, (int) bo[BO_LIN_LEAPS][0], (int) bo[BO_LIN_LEAPS][1], (int) bo[BO_LIN_LEAPS][2], (int) bo[BO_LIN_LEAPS][3], 'E');
            logMsg += " TIMEU LEAPS";
        }
        if (psatFrame->pageWord[9].hasData) {   //page word type 10 includes GST-GPST conversion params.
            rinex.setHdLnData(rinex.TIMC, rinex.TIMC_GAGP, bo[BO_LIN_TIMEG], 0, satNum);
            logMsg += " TIMEG";
        }
        plog->fine(logMsg);
        //clear satellite pageword storage
        psatFrame->hasData = false;
        for (int i = 0; i < GALINAV_MAXWORDS; ++i) psatFrame->pageWord[i].hasData = false;
    }
    else plog->finer(logMsg);
    return true;
}

/**readGALINNavMsg read a Galileo I/NAV navigation message from the navigation raw data file.
 * Data are stored in galInavSatFrame of the corresponding satNum.
 *
 * @param constId constellation identification (G, R, E, ...)
 * @param satNum satellite number in the constellation
 * @param sfrmNum subframe number
 * @param wordNum word type number
 * @param wd a place to store the bytes of the navigation message
 * @param logMsg is the loggin message
 * @return true if data succesful read, false otherwise
 */
bool GNSSdataFromGRD::readGALINNavMsg(char &constId, int &satNum, int &sfrmNum, int &wordNum, string &logMsg) {
    int status;     //the status of the navigation message:0=UNKNOWN, 1=PARITY PASSED, 2=REBUILT
    int msgSize;    //the message size in bytes
    //For Galileo I/NAV, each page contains 2 page parts, even and odd, with a total of 2x114 = 228 bits, (sync & tail excluded)
    // that should be fit into 29 bytes, with MSB first (skip B229-B232).
    unsigned int navMsg[GALINAV_MSGSIZE]; //to store GAL I/NAV message bytes from receiver
    try {
        //read MT_SATNAV_GAL_INAV message data
        if (fscanf(grdFile, "%d;%c%d;%d;%d;%d", &status, &constId, &satNum, &wordNum, &sfrmNum, &msgSize) != 6
            || status < 1
            || constId != 'E'
            || satNum < GAL_MINPRN
            || satNum > GAL_MAXPRN
            || msgSize != GALINAV_MSGSIZE) {
            plog->warning(logMsg + LOG_MSG_INMP + LOG_MSG_OSIZ);
            return false;
        }
        logMsg += " sat:" + to_string(satNum)+ " word:" + to_string(wordNum) + " subfr:" + to_string(sfrmNum);
        if (wordNum < 1 || wordNum > GALINAV_MAXWORDS) {
            plog->finer(logMsg + LOG_MSG_NAVIG);
            return false;
        }
        for (int i = 0; i < GALINAV_MSGSIZE; ++i) {
            if (fscanf(grdFile,";%X", navMsg+i) != 1) {
                plog->warning(logMsg + LOG_MSG_ERRO + LOG_MSG_INMP);
                return false;
            }
        }
        //pack message bytes (a page) into the GAL message word
        //TODO verify the following approach used: it works with Xiaomi MI8, but not tested with other devices
        //according Android classes doc. tail is removed, but MI8 only removes the one in the even page
        //the 29 bytes contain the following bits:
        //  1 e/o + 1 pt + 112 data 1/2 + 6 tail (?) + 1 e/o + 1 pt + 16 data 2/2 + 64 reserved 1 + 24 CRC + 8 reserved ...
        //and we have to extract data 1/2 and data 2/2 into a contiguos 128 bit stream:
        //1st; skip first 1 e/o + 1 pt and extract the following 32 x 4 = 128 bits (it is repetitive)
        GALINAVFrameData *psatFrame = &galInavSatFrame[satNum - 1];
        GALINAVpageData *pmsgWord = &psatFrame->pageWord[wordNum - 1];
        for (int i = 0, n=0; i < GALINAV_DATAW; i++, n+=4) {
            pmsgWord->data[i] = navMsg[n]<<26 | navMsg[n+1]<<18 | navMsg[n+2]<<10 | navMsg[n+3]<<2 | navMsg[n+4]>>6;
        }
        //2nd: but we have to remove the tail +1 e/o + 1 pt between data 1/2 and data 2/2 and add the new 6 bits from navMsg[16] and 2 from navMsg[17]
        pmsgWord->data[GALINAV_DATAW-1] = (pmsgWord->data[GALINAV_DATAW-1] & 0xFFFF0000)
                                          | ((pmsgWord->data[GALINAV_DATAW-1] & 0x000000FF) << 8)
                                          | ((navMsg[16] & 0x3F) << 2)
                                          | ((navMsg[17] & 0xC0) >> 6);
        //TODO analyse if CRC shall be checked (is it necessary or not?)
        psatFrame->hasData = true;
        pmsgWord->hasData = true;
        logMsg += " Word saved.";
    } catch (int error) {
        plog->severe(logMsg + LOG_MSG_ERRO + to_string((long long) error));
        return false;
    }
    return true;
}

/**extractGALINEphemeris extract satellite number and ephemeris from the given navigation message transmitted by GPS satellites.
 * <p>The navigation message data of interest here have been stored in GALILEO frame (galINAVSatFrame) message words.
 * Ephemeris are extracted from mesage words (see Galileo OS ICD for bit arrangement) and stored into a RINEX broadcast orbit like (bom) arrangement.
 * <p>This method stores the MANTISSA of each satellite ephemeris into a broadcast orbit array, without applying any scale factor.
 * <p>In addition to the 8 lines x 4 ephemeris per satellite needed for RINEX, for the header of the file they are extracted
 * the iono and time corrections, and the UTC leap seconds available in the navigation message. They are stored in the
 * rows BO_LIN_IONOA, BO_LIN_IONOB, BO_LIN_TIMEU, and BO_LIN_LEAP respectively.
 *
 * @param satIdx the satellite index in the gpsSatFrame
 * @param bom an array of broadcats orbit data containing the mantissa of each satellite ephemeris
 */
void GNSSdataFromGRD::extractGALINEphemeris(int satIdx, int (&bom)[BO_LINSTOTAL][BO_MAXCOLS]) {
    //use pointers for convenience
    GALINAVFrameData *pframe = &galInavSatFrame[satIdx];
    uint32_t *pw1data = pframe->pageWord[0].data;
    uint32_t *pw2data = pframe->pageWord[1].data;
    uint32_t *pw3data = pframe->pageWord[2].data;
    uint32_t *pw4data = pframe->pageWord[3].data;
    uint32_t *pw5data = pframe->pageWord[4].data;
    uint32_t *pw6data = pframe->pageWord[5].data;
    uint32_t *pw10data = pframe->pageWord[9].data;
    for (int i = 0; i < BO_LINSTOTAL; ++i) {
        for (int j = 0; j < BO_MAXCOLS; ++j) {
            bom[i][j] = 0;
        }
    }
    //broadcast line 0
    bom[0][0] = getBits(pw4data, GALIN_BIT(54), 14);                         //T0C
    bom[0][1] = getTwosComplement(getBits(pw4data, GALIN_BIT(68), 31), 31);  //Af0
    bom[0][2] = getTwosComplement(getBits(pw4data, GALIN_BIT(99), 21), 21);  //Af1
    bom[0][3] = getTwosComplement(getBits(pw4data, GALIN_BIT(120), 6), 6);   //Af2
    //broadcast line 1
    bom[1][0] = getBits(pw1data, GALIN_BIT(6), 10);	                        //IODnav
    bom[1][1] = getTwosComplement(getBits(pw3data, GALIN_BIT(104), 16), 16);//Crs
    bom[1][2] = getTwosComplement(getBits(pw3data, GALIN_BIT(40), 16), 16);	//Delta n
    bom[1][3] = getTwosComplement(getBits(pw1data, GALIN_BIT(30), 32),32);  //M0
    //broadcast line 2
    bom[2][0] = getTwosComplement(getBits(pw3data, GALIN_BIT(56), 16), 16);	//Cuc
    bom[2][1] = getBits(pw1data, GALIN_BIT(62), 32);                        //e
    bom[2][2] = getTwosComplement(getBits(pw3data, GALIN_BIT(72), 16), 16);	//Cus
    bom[2][3] = getBits(pw1data, GALIN_BIT(94), 32);   	                    //sqrt(A)
    //broadcast line 3
    bom[3][0] = getBits(pw1data, GALIN_BIT(16), 14);       	                //Toe
    bom[3][1] = getTwosComplement(getBits(pw4data, GALIN_BIT(22), 16), 16);	//Cic
    bom[3][2] = getTwosComplement(getBits(pw2data, GALIN_BIT(16), 32), 32); //OMEGA0
    bom[3][3] = getTwosComplement(getBits(pw4data, GALIN_BIT(38), 16), 16);	//CIS
    //broadcast line 4
    bom[4][0] = getTwosComplement(getBits(pw2data, GALIN_BIT(48), 32), 32); //i0
    bom[4][1] = getTwosComplement(getBits(pw3data, GALIN_BIT(88), 16), 16);	//Crc
    bom[4][2] = getTwosComplement(getBits(pw2data, GALIN_BIT(80), 32), 32); //w (omega)
    bom[4][3] = getTwosComplement(getBits(pw3data, GALIN_BIT(16), 24), 24); //w dot
    //broadcast line 5
    bom[5][0] = getTwosComplement(getBits(pw2data, GALIN_BIT(112), 14), 14);	//IDOT
    bom[5][1] = 0xA0400000;				        //data source: assumed non-exclusive and for E5b, E1
    bom[5][2] = getBits(pw5data, GALIN_BIT(73), 12) + 1024 + nGALrollOver * 4096;  //GAL week# (GST week + weeks to 1st GPS roll over + GAL roll over
    //broadcast line 6
    bom[6][0] = getBits(pw3data, GALIN_BIT(120), 8);			    //SISA
    bom[6][1] = (getBits(pw5data, GALIN_BIT(72), 1) << 31)      //SV health: E1B DVS
                | (getBits(pw5data, GALIN_BIT(69), 2) << 29)	//SV health: E1B HS
                | (getBits(pw5data, GALIN_BIT(71), 1) << 25)    //SV health: E5b DVS
                | (getBits(pw5data, GALIN_BIT(69), 2) << 23);   //SV health: E5b HS
    bom[6][2] = getTwosComplement(getBits(pw5data, GALIN_BIT(47), 10), 10);    //BGD E5a / E1
    bom[6][3] = getTwosComplement(getBits(pw5data, GALIN_BIT(57), 10), 10);    //BGD E5b / E1
    //broadcast line 7
    bom[7][0] = getBits(pw5data, GALIN_BIT(85), 20);	//Time of message: TOW from GST
    //extract corrections data
    bom[BO_LIN_IONOA][0] = getTwosComplement(getBits(pw5data, GALIN_BIT(6), 11), 11);   //Iono Ai0
    bom[BO_LIN_IONOA][1] = getTwosComplement(getBits(pw5data, GALIN_BIT(17), 11), 11);  //Iono Ai1
    bom[BO_LIN_IONOA][2] = getTwosComplement(getBits(pw5data, GALIN_BIT(28), 14), 14);  //Iono Ai2
    bom[BO_LIN_TIMEU][0] = getBits(pw6data, GALIN_BIT(6), 32);                          //GST - UTC correction A0
    bom[BO_LIN_TIMEU][1] = getTwosComplement(getBits(pw5data, GALIN_BIT(38), 24), 24);  //GST - UTC correction A1
    bom[BO_LIN_TIMEU][2] = getBits(pw5data, GALIN_BIT(70), 8);   //GST - UTC correction t0t
    bom[BO_LIN_TIMEU][3] = getBits(pw5data, GALIN_BIT(78), 8);   //GST - UTC correction WN0t
    bom[BO_LIN_TIMEU][3] |= bom[5][2] & (~MASK8b);       //put WNt as a continuous week number
    bom[BO_LIN_TIMEG][0] = getTwosComplement(getBits(pw10data, GALIN_BIT(86), 16), 16);     //Time correction AG0
    bom[BO_LIN_TIMEG][1] = getTwosComplement(getBits(pw10data, GALIN_BIT(102), 12), 12);    //Time correction AG1
    bom[BO_LIN_TIMEG][2] = getBits(pw10data, GALIN_BIT(114), 8);      //ref time: t0G
    bom[BO_LIN_TIMEG][3] = getBits(pw10data, GALIN_BIT(122), 6);      //ref week number: WN0G
    bom[BO_LIN_TIMEG][3] |= bom[5][2] & (~MASK8b);       //put WN0G as a continuous week number
    bom[BO_LIN_LEAPS][0] = getTwosComplement(getBits(pw6data, GALIN_BIT(62), 8), 8);    //delta tLS (leap seconds)
    bom[BO_LIN_LEAPS][1] = getTwosComplement(getBits(pw6data, GALIN_BIT(95), 8), 8);    //delta tLSF
    bom[BO_LIN_LEAPS][2] = getBits(pw6data, GALIN_BIT(86), 8);    //WN_LSF
    bom[BO_LIN_LEAPS][2] |= bom[5][2] & (~MASK8b);       //put WN_LSF as a continuous week number
    bom[BO_LIN_LEAPS][3] = getBits(pw6data, GALIN_BIT(92), 3);    //DN_LSF
}

/**scaleGALEphemeris apply scale factors to satellite ephemeris mantissas to obtain true satellite ephemeris and store them
 * in RINEX broadcast orbit like arrangements.
 * The given navigation data are the mantissa perameters as transmitted in the Galileo I/NAV navigation message, that shall be
 * converted to the true values applying the corresponding scale factors.
 * It is obtained a time tag from the week number and time of week inside the satellite ephemeris. Its value are the seconds
 * transcurred from the GPS epoch.
 *
 * @param bom the mantissas of orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @param bo the orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @return the time tag of the satellite ephemeris as seconds from the GPS epoch
 */
double GNSSdataFromGRD::scaleGALEphemeris(int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], double (&bo)[BO_LINSTOTAL][BO_MAXCOLS]) {
    for(int i=0; i<BO_LINSTOTAL; i++) {
        for(int j=0; j<BO_MAXCOLS; j++) {
            bo[i][j] = bom[i][j] * GAL_SCALEFACTOR[i][j];
        }
    }
    //recompute the ones being 32 bits twos complement signed integers
    bo[2][1] = ((unsigned int) bom[2][1]) * GAL_SCALEFACTOR[2][1];  //e
    bo[2][3] = ((unsigned int) bom[2][3]) * GAL_SCALEFACTOR[2][3];  //sqrt(A)
    bo[BO_LIN_TIMEU][0] = ((unsigned int) bom[BO_LIN_TIMEU][0]) * GAL_SCALEFACTOR[BO_LIN_TIMEU][0];  //A0
    //compute SISA value
    double value;
    int sisa = bom[6][0];
    if (sisa < 50) value = 0.01 * sisa;
    else if (sisa < 75) value = 0.5 + 0.02 * sisa;
    else if (sisa < 100) value = 1.0 + 0.04 * (sisa - 75);
    else if (sisa < 125) value = 2.0 + 0.16 * (sisa - 100);
    else if (sisa < 255) value = 0.0;   //spare
    else value = -1.0;
    bo[6][0] = value;
    return getInstantGNSStime( //use this function for convenience
            bom[5][2],	//GAL week# referred to GPS epoch
            bo[0][0]);  //T0c
}
#undef GALIN_BIT

//A macro to get bit position in a subframe from the bit position (BITNUMBER) in the message subframe stated in the GPS ICD
//Each GPS 30 bits message word is stored in an uint32_t (aligned to the rigth)
//Each GPS subframe, comprising 10 words, is tored in an stream of 10 uint32_t
//BITNUMBER is the position in the subframe, GPSL1CA_BIT gets the position in the stream
#define BDSD1_BIT(BITNUMBER) ((BITNUMBER-1)/30*32 + (BITNUMBER-1)%30 + 2)

/**collectBDSD1Ephemeris gets BDS D1 navigation data from a raw message and store them to generate RINEX navigation files.
 * Satellite navigation messages are temporary stored until all needed subframes for the current satellite and epoch have been received.
 * When completed, satellite ephemeris are extracted from subframes, scaled to working units, and stored into "broadcast orbit lines"
 * of the RinexData object.
 *
 * @param rinex	the class instance where data are stored
 * @param msgType the true type of the message
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectBDSD1Ephemeris(RinexData &rinex, int msgType) {
    //TODO test this method with real data
    char constId;   //the constellation identifier this satellite belongs
    int satNum;		//the satellite number this navigation message belongssatt
    int sfrmNum;    //navigation message subframe number
    int pageNum;    //navigation message page number
    int bom[BO_LINSTOTAL][BO_MAXCOLS];		//a RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
    double bo[BO_LINSTOTAL][BO_MAXCOLS];	//the RINEX broadcats orbit arrangement for satellite ephemeris
    double tTag;		//the time tag for ephemeris data
    BDSD1FrameData *pframe;
    string logMsg = getMsgDescription(msgType);
    //read MT_SATNAV_BEIDOU_D1 message data
    if (!readBDSD1NavMsg(constId, satNum, sfrmNum, pageNum, logMsg)) return false;
    //check if all ephemerides have been received, that is, all subframes have data
    pframe = &bdsSatFrame[satNum - 1];
    bool allRec = pframe->bdsSatSubframes[0].hasData;
    for (int i = 1; i < BDSD1_MAXSUBFRS; ++i) allRec = allRec && pframe->bdsSatSubframes[i].hasData;
    if (allRec) {
        logMsg += LOG_MSG_FRM;
        //all ephemerides have been already received; extract and store them into the RINEX object
        plog->fine(logMsg);
        extractBDSD1Ephemeris(satNum - 1, bom);
        tTag = scaleBDSEphemeris(bom, bo);
        rinex.saveNavData('C', satNum, bo, tTag);
        //clear satellite frame storage
        pframe->hasData = false;
        for (int i = 0; i < BDSD1_MAXSUBFRS; ++i) pframe->bdsSatSubframes[i].hasData = false;
    } else plog->finer(logMsg);
    return true;
}

/**collectBDSD1Corrections gets BDS D1 navigation data from raw data message the ionospheric, clock and leap corrections data
 * needed to generate some RINEX navigation files header records.
 * Satellite navigation messages are temporary stored until a subframe 1 or 5 (page 9 or 10) 10 has been received.
 * When received, corrections are extracted from pages, scaled to working units, and stored into header records of the RinexData object.
 *
 * @param rinex	the class instance where data are stored
 * @param msgType the true type of the message
 * @return true if data properly extracted (data correctly read, correct parity status, relevant subframe), false otherwise
 */
bool GNSSdataFromGRD::collectBDSD1Corrections(RinexData &rinex, int msgType) {
    //TODO test this method with real data
    int status;     //the status of the navigation message:0=UNKNOWN, 1=PARITY PASSED, 2=REBUILT
    char constId;   //the constellation identifier this satellite belongs
    int satNum;		//the satellite number this navigation message belongssatt
    int sfrmNum;    //navigation message subframe number
    int pageNum;    //navigation message page number
    int msgSize;    //the message size in bytes
    uint32_t wd[BDSD1_SUBFRWORDS];  //a place to store the subframe words
    int bom[BO_LINSTOTAL][BO_MAXCOLS];		//a RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
    double bo[BO_LINSTOTAL][BO_MAXCOLS];	//the RINEX broadcats orbit arrangement for satellite ephemeris
    double tTag;		//the time tag for ephemeris data
    BDSD1SubframeData *psubframe;
    BDSD1FrameData *pframe;
    string logMsg = getMsgDescription(msgType);
    //read MT_SATNAV_BEIDOU_D1 message data
    if (!readBDSD1NavMsg(constId, satNum, sfrmNum, pageNum, logMsg)) return false;
    //check if corrections have been received
    pframe = &bdsSatFrame[satNum - 1];
    if (pframe->hasData &&
        (pframe->bdsSatSubframes[0].hasData
        || pframe->bdsSatSubframes[3].hasData
        || pframe->bdsSatSubframes[4].hasData)) {
        logMsg += LOG_MSG_CORR;
        extractBDSD1Ephemeris(satNum - 1, bom);
        tTag = scaleBDSEphemeris(bom, bo);
        if (pframe->bdsSatSubframes[0].hasData) {
            rinex.setHdLnData(rinex.IONC, rinex.IONC_BDSA, bo[BO_LIN_IONOA], 0, satNum);
            rinex.setHdLnData(rinex.IONC, rinex.IONC_BDSB, bo[BO_LIN_IONOB], 0, satNum);
            logMsg += "IONA&B";
        }
        if (pframe->bdsSatSubframes[4].hasData) {
            rinex.setHdLnData(rinex.TIMC, rinex.TIMC_BDUT, bo[BO_LIN_TIMEU], 0, satNum);
            logMsg += " TIMEU";
            if (pframe->bdsSatSubframes[0].hasData) {
                rinex.setHdLnData(rinex.LEAP, (int) bo[BO_LIN_LEAPS][0], (int) bo[BO_LIN_LEAPS][1], (int) bo[BO_LIN_LEAPS][2], (int) bo[BO_LIN_LEAPS][3], 'C');
                logMsg += " LEAPS";
            }
        }
        if (pframe->bdsSatSubframes[3].hasData) {
            rinex.setHdLnData(rinex.TIMC, rinex.TIMC_BDGP, bo[BO_LIN_TIMEG], 0, satNum);
            pframe->bdsSatSubframes[3].hasData = false;
            logMsg += " TIMEG";
        }
        plog->fine(logMsg);
        //clear satellite frame storage. Note that subframe flags are note reset because LEAPS needs subframes 1 and 5 page 10
        pframe->hasData = false;
    } else plog->finer(logMsg);
    return true;
}

/**
 * readBDSD1NavMsg read a Beidou D1 navigation message from the navigation raw data file.
 * Data are stored in bdsSatSubframes of the corresponding satNum.
 * For Beidou D1, each subframe of the navigation message contains 10 30-bit words.
 * Each word (30 bits) should be fit into the last 30 bits in a 4-byte word (skip B31 and B32), with MSB first,
 * for a total of 40 bytes
 * Only have interest subframes: 1,2,3 & page 18 of subframe 4 (pgID = 56 in GPS ICD Table 20-V)
 *
 * @param constId constellation identification (G, R, E, ...)
 * @param satNum satellite number in the constellation
 * @param sfrmNum subframe number
 * @param pageNum page number
 * @param wd a place to store the bytes of the navigation message
 * @param logMsg is the loggin message
 * @return true if data succesful read, false otherwise
 */
bool GNSSdataFromGRD::readBDSD1NavMsg(char &constId, int &satNum, int &sfrmNum, int &pageNum, string &logMsg) {
    //TODO test this method with real data
    int status;     //the status of the navigation message:0=UNKNOWN, 1=PARITY PASSED, 2=REBUILT
    int msgSize;    //the message size in bytes
    unsigned int navMsg[BDSD1_MSGSIZE]; //to store BDS message bytes from receiver
    BDSD1SubframeData *psubframe;
    BDSD1FrameData *pframe;
    try {
        //read MT_SATNAV_BEIDOU_D1 message data
        if (fscanf(grdFile, "%d;%c%d;%d;%d;%d", &status, &constId, &satNum, &sfrmNum, &pageNum, &msgSize) != 6
            || status < 1
            || constId != 'C'
            || satNum < BDS_MINPRN
            || satNum > BDS_MAXPRN
            || msgSize != BDSD1_MSGSIZE) {
            plog->warning(logMsg + LOG_MSG_INMP + LOG_MSG_OSIZ);
            return false;
        }
        logMsg += " sat:" + to_string(satNum) + " subfr:" + to_string(sfrmNum) + " pg:" + to_string(pageNum);
        //only subframes 1, 2, 4 and pages 9 and 10 of subframe 5 have data for RINEX files
        if (sfrmNum != 1 && sfrmNum != 2 && sfrmNum != 3 && (sfrmNum != 5 || (pageNum != 9 && pageNum != 10))) {
            plog->finer(logMsg + LOG_MSG_NAVIG);
            return false;
        }
        //read message bytes
        for (int i = 0; i < BDSD1_MSGSIZE; ++i) {
            if (fscanf(grdFile,";%X", navMsg+i) != 1) {
                plog->warning(logMsg + LOG_MSG_ERRO + LOG_MSG_INMP);
                return false;
            }
        }
        //pack message bytes into the ten words with navigation data
        pframe = &bdsSatFrame[satNum - 1];
        //set index for subframes:0 is subframe 1; 1 is 2; 2 is 3; 3 is subfr 5 page 9; 4 is subfr 5 page 10
        psubframe = &pframe->bdsSatSubframes[sfrmNum - 1];
        if (sfrmNum == 5 && pageNum == 9) psubframe = &pframe->bdsSatSubframes[3];
        for (int i = 0, n=0; i < BDSD1_SUBFRWORDS; ++i, n+=4) {
            psubframe->words[i] = navMsg[n]<<24 | navMsg[n+1]<<16 | navMsg[n+2]<<8 | navMsg[n+3];
        }
        psubframe->hasData = true;
        pframe->hasData = true;
        logMsg += LOG_MSG_SFR;
        //TODO is it necessary to check for SVhealth?
    } catch (int error) {
        plog->severe(logMsg + LOG_MSG_ERRO + to_string((long long) error));
        return false;
    }
    return true;
}

/**extractBDSD1Ephemeris extract satellite number and ephemeris from the given D1 navigation message transmitted by BDS satellites.
 * <p>The navigation message data of interest here have been stored in BDS frame (bdsSatFrame) data words (without parity).
 * Ephemeris are extracted from this array and stored into a RINEX broadcast orbit like (bom) arrangement.
 * <p>This method stores the MANTISSA of each satellite ephemeris into a broadcast orbit array, without applying any scale factor.
 * <p>In addition to the 8 lines x 4 ephemeris per satellite needed for RINEX, for the header of the file they are extracted
 * the iono and time corrections, and the UTC leap seconds available in the navigation message. They are stored in the
 * rows BO_LIN_IONOA, BO_LIN_IONOB, BO_LIN_TIMEx, and BO_LIN_LEAP respectively.
 *
 * @param satIdx the satellite index in the gpsSatFrame
 * @param bom an array of broadcats orbit data containing the mantissa of each satellite ephemeris
 */
void GNSSdataFromGRD::extractBDSD1Ephemeris(int satIdx, int (&bom)[BO_LINSTOTAL][BO_MAXCOLS]) {
    //TODO test this method with real data
    uint32_t *psubfr1data = bdsSatFrame[satIdx].bdsSatSubframes[0].words;
    uint32_t *psubfr2data = bdsSatFrame[satIdx].bdsSatSubframes[1].words;
    uint32_t *psubfr3data = bdsSatFrame[satIdx].bdsSatSubframes[2].words;
    uint32_t *psubfr5_9data = bdsSatFrame[satIdx].bdsSatSubframes[3].words;   //subframe 5, page 9
    uint32_t *psubfr5_10data = bdsSatFrame[satIdx].bdsSatSubframes[4].words;   //subframe 5, page 10
    for (int i = 0; i < BO_LINSTOTAL; ++i) {
        for (int j = 0; j < BO_MAXCOLS; ++j) {
            bom[i][j] = 0;
        }
    }
    //broadcast line 0
    bom[0][0] = (getBits(psubfr1data, BDSD1_BIT(74), 9) << 8) | getBits(psubfr1data, BDSD1_BIT(91), 8); //T0C
    bom[0][1] = getTwosComplement((getBits(psubfr1data, BDSD1_BIT(226), 7) << 17) | getBits(psubfr1data, BDSD1_BIT(241), 17), 24);	//Af0
    bom[0][2] = getTwosComplement((getBits(psubfr1data, BDSD1_BIT(258), 5) << 17) | getBits(psubfr1data, BDSD1_BIT(241), 17), 22);	//Af1
    bom[0][3] = getTwosComplement(getBits(psubfr1data, BDSD1_BIT(215), 11), 11);    	//Af2
    //broadcast line 1
    bom[1][0] = getBits(psubfr1data, BDSD1_BIT(288), 5);	                    //IODE (AODE)
    bom[1][1] = getTwosComplement((getBits(psubfr2data, BDSD1_BIT(225), 4) << 10) | getBits(psubfr2data, BDSD1_BIT(241), 10), 14); //Crs
    bom[1][2] = getTwosComplement((getBits(psubfr2data, BDSD1_BIT(43), 10) << 6) | getBits(psubfr2data, BDSD1_BIT(61), 6), 16);	//Delta n
    bom[1][3] = (getBits(psubfr2data, BDSD1_BIT(93), 20) << 12) | getBits(psubfr2data, BDSD1_BIT(121), 12);//M0
    //broadcast line 2
    bom[2][0] = getTwosComplement((getBits(psubfr2data, BDSD1_BIT(67), 16) << 2) | getBits(psubfr2data, BDSD1_BIT(91), 2), 18); //Cuc
    bom[2][1] = (getBits(psubfr2data, BDSD1_BIT(133), 10) << 22) | getBits(psubfr2data, BDSD1_BIT(151), 25);	//e
    bom[2][2] = getTwosComplement(getBits(psubfr2data, BDSD1_BIT(181), 18), 18);                                //Cus
    bom[2][3] = (getBits(psubfr2data, BDSD1_BIT(251), 12) << 20) | getBits(psubfr2data, BDSD1_BIT(271), 20);	//sqrt(A)
    //broadcast line 3
    bom[3][0] = (getBits(psubfr2data, BDSD1_BIT(291), 2) << 15) | (getBits(psubfr3data, BDSD1_BIT(43), 10) << 5) | getBits(psubfr3data, BDSD1_BIT(61), 5);                            //Toe
    bom[3][1] = getTwosComplement((getBits(psubfr3data, BDSD1_BIT(106), 7) << 11) | getBits(psubfr3data, BDSD1_BIT(121), 11), 18);  //Cic
    bom[3][2] = (getBits(psubfr3data, BDSD1_BIT(212), 21) << 11) | getBits(psubfr3data, BDSD1_BIT(241), 11);                     	//OMEGA0
    bom[3][3] = getTwosComplement((getBits(psubfr3data, BDSD1_BIT(164), 9) << 9) | getBits(psubfr3data, BDSD1_BIT(181), 9), 18);	//CIS
    //broadcast line 4
    bom[4][0] = (getBits(psubfr3data, BDSD1_BIT(66), 17) << 15) | getBits(psubfr3data, BDSD1_BIT(91), 15);                          //i0
    bom[4][1] = getTwosComplement((getBits(psubfr2data, BDSD1_BIT(199), 4) << 14) | getBits(psubfr2data, BDSD1_BIT(211), 14), 18);	//Crc
    bom[4][2] = (getBits(psubfr3data, BDSD1_BIT(252), 11) << 21) | getBits(psubfr3data, BDSD1_BIT(271), 21);                    	//w (omega)
    bom[4][3] = getTwosComplement((getBits(psubfr3data, BDSD1_BIT(132), 11) << 13) | getBits(psubfr3data, BDSD1_BIT(151), 13), 24); //w dot
    //broadcast line 5
    bom[5][0] = getTwosComplement((getBits(psubfr3data, BDSD1_BIT(190), 13) << 1) | getBits(psubfr3data, BDSD1_BIT(211), 1) , 14);	//IDOT
    bom[5][2] = getBits(psubfr1data, BDSD1_BIT(61), 13) + nBDSrollOver * 8192; //BDS week
    //broadcast line 6
    bom[6][0] = getBits(psubfr1data, BDSD1_BIT(49), 4);			//URA index
    bom[6][1] = getBits(psubfr1data, BDSD1_BIT(43), 1);      	//sat H1
    bom[6][2] = getTwosComplement(getBits(psubfr1data, BDSD1_BIT(99), 10), 10);                                                     //TGD1 B1/B3
    bom[6][3] = getTwosComplement((getBits(psubfr1data, BDSD1_BIT(109), 4) << 6) | getBits(psubfr1data, BDSD1_BIT(121), 6), 10);	//TGD2 B2/B3
    //broadcast line 7
    bom[7][0] = (getBits(psubfr1data, BDSD1_BIT(19), 8) << 12) | getBits(psubfr1data, BDSD1_BIT(31), 12); //Transmission time of message: SOW
    bom[7][1] = getBits(psubfr1data, BDSD1_BIT(44), 5);			//IODC (AODC
    //extract corrections data
    bom[BO_LIN_IONOA][0] = getTwosComplement(getBits(psubfr1data, BDSD1_BIT(127), 8), 8);    //alfa0
    bom[BO_LIN_IONOA][1] = getTwosComplement(getBits(psubfr1data, BDSD1_BIT(135), 8), 8);    //alfa1
    bom[BO_LIN_IONOA][2] = getTwosComplement(getBits(psubfr1data, BDSD1_BIT(151), 8), 8);    //alfa2
    bom[BO_LIN_IONOA][3] = getTwosComplement(getBits(psubfr1data, BDSD1_BIT(159), 8), 8);    //alfa3
    bom[BO_LIN_IONOB][0] = getTwosComplement((getBits(psubfr1data, BDSD1_BIT(167), 6) << 2) | getBits(psubfr1data, BDSD1_BIT(181), 2), 8);   //beta0
    bom[BO_LIN_IONOB][1] = getTwosComplement(getBits(psubfr1data, BDSD1_BIT(183), 8), 8);   //beta1
    bom[BO_LIN_IONOB][2] = getTwosComplement(getBits(psubfr1data, BDSD1_BIT(191), 8), 8);   //beta2
    bom[BO_LIN_IONOB][3] = getTwosComplement((getBits(psubfr1data, BDSD1_BIT(199), 4)<<4) | getBits(psubfr1data, BDSD1_BIT(211), 4), 8);   //beta3
    bom[BO_LIN_TIMEU][0] = (getBits(psubfr5_10data, BDSD1_BIT(91), 22) << 10) | getBits(psubfr5_10data, BDSD1_BIT(121), 10);    //Time correction A0UTC
    bom[BO_LIN_TIMEU][1] = getTwosComplement((getBits(psubfr5_10data, BDSD1_BIT(131), 12) << 12) | getBits(psubfr5_10data, BDSD1_BIT(151), 12), 24);    //A1UTC
    bom[BO_LIN_TIMEG][0] = getTwosComplement(getBits(psubfr5_9data, BDSD1_BIT(97), 14), 14);      //A0GPS
    bom[BO_LIN_TIMEG][1] = getTwosComplement((getBits(psubfr5_9data, BDSD1_BIT(111), 2) << 14) | getBits(psubfr5_9data, BDSD1_BIT(121), 14), 16);    //A1GPS
    bom[BO_LIN_LEAPS][0] = getTwosComplement((getBits(psubfr5_10data, BDSD1_BIT(51), 2) << 6) | getBits(psubfr5_10data, BDSD1_BIT(61), 6) , 8);    //delta tLS (leap seconds)
    bom[BO_LIN_LEAPS][1] = getTwosComplement(getBits(psubfr5_10data, BDSD1_BIT(67), 8), 8);    //delta tLSF
    bom[BO_LIN_LEAPS][2] = getBits(psubfr5_10data, BDSD1_BIT(75), 8);    //WN_LSF
    bom[BO_LIN_LEAPS][2] |= bom[5][2] & (~MASK8b);       //put WN_LSF as a continuous week number
    bom[BO_LIN_LEAPS][3] = getBits(psubfr5_10data, BDSD1_BIT(163), 8);    //DN_LSF
}
#undef BDSD1_BIT

/**scaleBDSEphemeris  apply scale factors to satellite ephemeris mantissas to obtain true satellite ephemeris and store them
 * in RINEX broadcast orbit like arrangements.
 * The given navigation data are the mantissa parameters as transmitted in the BDS navigation message, that shall be
 * converted to the true values applying the corresponding scale factors.
 * It is obtained a time tag from the week number and time of week inside the satellite ephemeris. Its value are the seconds
 * transcurred from the GPS epoch.
 *
 * @param bom the mantissas of orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @param bo the orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each
 * @return the time tag of the satellite ephemeris as seconds from the GPS epoch
 */
double GNSSdataFromGRD::scaleBDSEphemeris(int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], double (&bo)[BO_LINSTOTAL][BO_MAXCOLS]) {
    //TODO test this method with real data
    double aDouble;
    int iodc = bom[6][3];
    for(int i=0; i<BO_LINSTOTAL; i++) {
        for (int j = 0; j <BO_MAXCOLS; j++) {
            bo[i][j] = bom[i][j] * BDS_SCALEFACTOR[i][j];
        }
    }
    //recompute the ones being 32 bits twos complement signed integers
    bo[2][1] = ((unsigned int) bom[2][1]) * BDS_SCALEFACTOR[2][1];  //e
    bo[2][3] = ((unsigned int) bom[2][3]) * BDS_SCALEFACTOR[2][3];  //sqrt(A)
    bo[BO_LIN_TIMEU][0] = ((unsigned int) bom[BO_LIN_TIMEU][0]) * BDS_SCALEFACTOR[BO_LIN_TIMEU][0];  //A0
    //compute User Range Accuracy value
    if (bom[6][0] == 1) bo[6][0] = 2.8;
    else if (bom[6][0] == 3) bo[6][0] = 5.7;
    else if (bom[6][0] == 5) bo[6][0] = 11.3;
    else if (bom[6][0] < 6) bo[6][0] = pow(2.0, bom[6][0]/2 + 1);
    else if (bom[6][0] < 15) bo[6][0] = pow(2.0, bom[6][0] - 2);
    else bo[6][0] = 0.0;
    return getInstantGNSStime(
            bom[5][2] + 1356,	//BDS week# referred to GPS epoch
            bo[0][0]) + 14;     //T0c plus leap at BDS epoch
}

/**isGoodGRDver check if the given GRD type and version can be processed by the current version of the class.
 *
 * @param identification is the file type identification
 * @param version is file version
 * @return true if this file version can be processed by the current version of this class, false otherwise
 */
bool GNSSdataFromGRD::isGoodGRDver(string identification, int version) {
    //check the type
    if (ORD_FILE_EXTENSION.compare(identification) == 0) {
        if ((version < MIN_ORD_FILE_VERSION) || (version > MAX_ORD_FILE_VERSION)) return false;
        ordVersion = version;
        return true;
    } else if (NRD_FILE_EXTENSION.compare(identification) == 0) {
        if ((version < MIN_NRD_FILE_VERSION) || (version > MAX_NRD_FILE_VERSION)) return false;
        nrdVersion = version;
        return true;
    }
    return false;
}

/**addSignal adds the given signal (1C,2C,1A,1S,1L,1X, etc) to the given system in the list of GNSSsystem.
 * <p>If the given system does not exist in the list, it is added.
 * <p>If the given signal does not exist in the given system, it is added.
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
            sgnl.clear();
            for (vector<string>::iterator itt = it->obsType.begin(); itt != it->obsType.end(); ++itt) {
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
 * @return true
 */
bool GNSSdataFromGRD::trimBuffer(char* cstrToTrim, const char* unwanted) {
    char* last = cstrToTrim + strlen(cstrToTrim) - 1;
    while ((last > cstrToTrim) && strchr(unwanted, (int) *last) != NULL) {
        *last = 0;
        last--;
    }
    return true;
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
void GNSSdataFromGRD::llaTOxyz( const double lat, const double lon, const double alt, double &x, double &y, double &z) {
    double sinlat = sin(lat);
    double coslat = cos(lat);
    //rn is the radius of curvature in the prime vertical
    double rn = ECEF_A / sqrt(1.0 - ECEF_E2 * pow(sinlat, 2));
    x = (rn + alt) * coslat * cos(lon);    //ECEF x
    y = (rn + alt) * coslat * sin(lon);    //ECEF y
    z = (rn * (1.0 - ECEF_E2) + alt) * sinlat;  //ECEF z
}

/**collectAndSetEpochTime is called just after reading message identifier MT_EPOCH to read data
 * in the rest of the message.
 * Data contained in the message are used to compute the epoch time, which is given as the week number
 * and the time of week (seconds from the beginning of this week.
 * <p> In the current version clock time is referenced to the GPS constellation time frame.
 * <p>The epoch time computed is used to set the current epoch time in the RinexData class.
 * <p>To compute the epoch time the receiver clock bias is taken into account or not, depending on
 * flag applyBias (value set by user). See RINEX V304 document on RCV CLOCK OFFS APPL record:
 * <p>- if applyBias is true, bias computed by receiver after the fix are subtracted from the receiver hardware
 * clock to obtain the epoch time.
 * <p>. if applyBias is false, bias are not applied to the receiver hardware.
 *
 * @param rinex the RinexData class where curren epoch ti9me will be set
 * @param tow the time of week, that is, the seconds from the beginning of the current week
 * @param numObs number of MT_SATOBS messages that will follow this one
 * @param logMsg a message to log
 * @return time of week in nanoseconds from the begining of the current week
 */
double GNSSdataFromGRD::collectAndSetEpochTime(RinexData& rinex, double& tow, int& numObs, string logMsg) {
    long long timeNanos = 0;        //the receiver hardware clock time
    long long fullBiasNanos = 0;    //difference between hardware clock and GPS time (tGPS = timeNanos - fullBiasNanos - biasNanos
    double biasNanos = 0.0;         //hardware clock sub-nano bias
    double driftNanos = 0.0;    //drift of biasNanos in nanos per second
    double tRx;  //receiver clock nanos from the beginning of the current week (using GPS time system)
    int clkDiscont = 0;
    int leapSeconds = 0;
    int eflag = 0;  //0=OK; 1=power failure happened
    int week;       //the current GPS week number
    numObs = 0;
    if (fscanf(grdFile, "%lld;%lld;%lf;%lf;%d;%d;%d", &timeNanos, &fullBiasNanos, &biasNanos, &driftNanos,
               &clkDiscont, &leapSeconds, &numObs) != 7) {
        plog->warning(logMsg + LOG_MSG_PARERR);
    }
    //Compute time references and set epoch time
    //Note that a double has a 15 digits mantisa. It is not sufficient for time nanos computation when counting
    //from beginning of GPS time, but it is sufficient when counting nanos from the beginning of current week (< 604,800,000,000,000 )
    timeNanos -= fullBiasNanos; //true time of the receiver
    week = (int) (timeNanos / NUMBER_NANOSECONDS_WEEK);
    timeNanos %= NUMBER_NANOSECONDS_WEEK;
    tRx = (double) timeNanos;
    if (applyBias) {
        tRx +=  biasNanos;
        while (tRx > (double) NUMBER_NANOSECONDS_WEEK) {
            week++;
            tRx -= (double) NUMBER_NANOSECONDS_WEEK;
        }
    }
    tow = tRx * 1E-9;   //tow in seconds
    if (clockDiscontinuityCount != clkDiscont) {
        eflag = 1;
        clockDiscontinuityCount = clkDiscont;
    }
    rinex.setEpochTime(week, tow, biasNanos * 1E-9, eflag);
    plog->fine(logMsg + " w=" + to_string(week) + " tow=" + to_string(tow)  + " applyBias:" + (applyBias?string("TRUE"):string("FALSE")));
    return tRx;
}

/**getMsgDescription provides a textual description of the message type (in raw data files or arguments) passed.
 *
 * @param msgt the message type to describe. The description of each message type is in the table msgTblTypes.
 * @return a string with the description found
 */
string GNSSdataFromGRD::getMsgDescription(int msgt) {
    int i = 0;
    while((msgTblTypes[i].type != msgt)
          && (msgTblTypes[i].type != MT_LAST))
        i++;
    return msgTblTypes[i].description + LOG_MSG_COUNT + ":";
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

/**isPsAmbiguous determines if the measurement associated to the provided synchState is ambiguous or not.
 * Also, if it is not ambiguous, recomputes the related values for receiver and received time clock depending on
 * constellation and synchronisation state. They are recomputed when the tracking state deduced from the
 * synchronisation state allows determination of unambiguous pseudorange.
 *
 * @param constellId the constellation identifier
 * @param signalId the three char identifier of the signal
 * @param synchState the synchronisation state given by Android getState for GNSS measurements
 * @param tRx the receiver clock in nanosececonds from the beginning of the current GPS week
 * @param tRxGNSS the receiver clock in the time frame of the given constellation and for the sync item
 * @param tTx initially the satellite transmitted clock, finally recomputed for the sync item
 * @return true if the synchronisation state will allow computation of unambiguous pseudoranges, false otherwise
 */
bool GNSSdataFromGRD::isPsAmbiguous(char constellId, char* signalId, int synchState, double tRx, double &tRxGNSS, long long &tTx) {
    bool psAmbiguous = false;
    tRxGNSS = tRx;      //by default it is assumed the GPS TOW
    switch (constellId) {
        case 'G':
        case 'J':
        case 'S':
            if (((synchState & ST_TOW_DECODED) != 0)
                && ((synchState & ST_CBSS_SYNC) != 0)) break;   //the time origin is the default start of week
            if (((synchState & ST_SUBFRAME_SYNC) != 0)) {      //the time origin is the start of the 6 sec. subframe
                tRxGNSS = fmod(tRx, NUMBER_NANOSECONDS_6S);
                tTx %= NUMBER_NANOSECONDS_6S;
                break;
            }
            //other trackStates will provide ambiguous pseudorange (not valid)
            psAmbiguous = true;
            break;
        case 'R':
            if (((synchState & ST_GLO_TOD_DECODED) != 0)
                && ((synchState & ST_CBGSS_SYNC) != 0)) {   //the time origin is the start of GLONASS day
                tRxGNSS = fmod(tRx + NUMBER_NANOSECONDS_3H - NUMBER_NANOSECONDS_18S, NUMBER_NANOSECONDS_DAY);
                tTx %= NUMBER_NANOSECONDS_DAY;
                break;
            }
            if ((synchState & ST_GLO_STRING_SYNC) != 0) {  //the time origin is the start of 2 sec. string
                tRxGNSS = fmod(tRx, NUMBER_NANOSECONDS_2S);
                tTx %= NUMBER_NANOSECONDS_2S;
                break;
            }
            //other trackStates will provide ambiguous pseudorange (not valid)
            psAmbiguous = true;
            break;
        case 'E':
            if (((synchState & ST_TOW_DECODED) != 0)
                && ((synchState & ST_CBSS_SYNC) != 0)) break;   //the time origin is the default start of week
            if (((synchState & ST_TOW_DECODED) != 0)
                && ((*(signalId) == '1') && (synchState & ST_GAL_E1BC_SYNC) != 0)) break;   //idem
            if ((synchState & ST_GAL_E1B_PAGE_SYNC) != 0) {      //the time origin is the start of the 2 sec. page
                tRxGNSS = fmod(tRx, NUMBER_NANOSECONDS_2S);
                tTx %= NUMBER_NANOSECONDS_2S;
                break;
            }
            if ((synchState & ST_GAL_E1C_2ND_CODE_LOCK) != 0) { //the time origin is the start of the 100msec 2nd code
                tRxGNSS = fmod(tRx, NUMBER_NANOSECONDS_100MS);
                tTx %= NUMBER_NANOSECONDS_100MS;
                break;
            }
            //other trackStates will provide ambiguous pseudorange (not valid)
            psAmbiguous = true;
            break;
        case 'C':
            if (((synchState & ST_TOW_DECODED) != 0)
                && ((synchState & ST_CBSS_SYNC) != 0)) {   //the time origin is the start of the BDS week
                tRxGNSS = fmod(tRx - NUMBER_NANOSECONDS_14S, NUMBER_NANOSECONDS_WEEK);  //BDS time = GPS time - 14s
                break;
            }
            if (((synchState & ST_SUBFRAME_SYNC) != 0)) {      //the time origin is the start of the 6 sec.subframe
                tRxGNSS = fmod(tRx - NUMBER_NANOSECONDS_14S, NUMBER_NANOSECONDS_6S);
                tTx %= NUMBER_NANOSECONDS_6S;
                break;
            }
            //other trackStates will provide ambiguous pseudorange (not valid)
            psAmbiguous = true;
            break;
        default:
            psAmbiguous = true;
            break;
    }
    return psAmbiguous;
}

/**
 * isCarrierPhInvalid determines if the carrier phase state will provide phase measurements valid for this constellation and signal.
 *
 * @param constellId the constellation identifier
 * @param signalId the three char identifier of the signal
 * @param carrierPhaseState the accumulated delta range state given by Android for GNSS measurements
 * @return true if the ADR state will allow computation of phase measurements, false otherwise
 *
 */
bool GNSSdataFromGRD::isCarrierPhInvalid(char constellId, char* signalId, int carrierPhaseState) {
    return carrierPhaseState == ST_UNKNOWN;
}

/**
 * isKnownMeasur checks validity of identifiers for constellation, satellite, band and signal attribute.
 *
 * @param constellId the constellation identifier
 * @param satNum the satellite number
 * @param frqId a frequency identifier
 * @param attribute the signal attribute
 * @return true if the satellite number is valid or has been converted to a valid one, false otherwise
 */
bool GNSSdataFromGRD::isKnownMeasur(char constellId, int satNum, char frqId, char attribute) {
    //check band and signal attribute
    if ((frqId == '?') || (attribute == '?')) return false;
    //check constellation and satellite number
    switch (constellId) {
        case 'R':
            if (satNum >= GLO_MINOSN && satNum <= GLO_MAXOSN) return true;
            if (satNum >= GLO_MINFCN && satNum <= GLO_MAXFCN) return true;
            break;
        case 'G':
            if (satNum >= GPS_MINPRN && satNum <= GPS_MAXPRN) return true;
            break;
        case 'C':
            if (satNum >= BDS_MINPRN && satNum <= BDS_MAXPRN) return true;
            break;
        case 'J':
            if (satNum >= QZSS_MINPRN && satNum <= QZSS_MAXPRN) return true;
            break;
        case 'S':
            if (satNum >= SBAS_MINPRN && satNum <= SBAS_MAXPRN) return true;
            break;
        case 'E':
            if (satNum >= GAL_MINPRN && satNum <= GAL_MAXPRN) return true;
            break;
        default:
            break;
    }
   return false;
}

/*getBits gets a number of bits starting at a given position of the bits stream passed.
 * The bit stream is an array of uint32_t words, being bit position 0 of the stream the bit 0 of word 0 (the left most),
 * position 1 of the stream the bit 1 of word 0, and so on from left to right.
 * The extracted bits are returned in a uint32_t with the right most bit extracted from the stream aligned to the
 * right most bit of the word.
 *
 * @param stream the array of unsigned ints containing the bit stream
 * @param bitpos the position in the stream of first bit to extract (first left bit)
 * @param len the number of bits to extract. It shall not be greater than the number of bits in an uint32_t
 * @return an uint32_t with the extracted bits aligned to the right
 */
uint32_t GNSSdataFromGRD::getBits(uint32_t *stream, int bitpos, int len) {
    uint32_t bitMask;
    uint32_t bits = 0;
    for (int i = bitpos; i < bitpos + len; i++) {
        bits = bits << 1;
        bitMask = 0x01U << (31 - (i % 32));
        if ((stream[i/32] & bitMask) != 0) bits |= 0x01;
    }
    return bits;
}

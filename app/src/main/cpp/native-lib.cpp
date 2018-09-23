#include <jni.h>
#include <string>
#include <string.h>
#include <stdio.h>

#include "Logger.h"
#include "GNSSdataFromGRD.h"
#include "RinexData.h"

//to log or give state
const string LOG_FILENAME = "LogFile.txt";
const string LOG_STARTMSG = "START GENERATE RINEX FILE";
const string LOG_MSG_HDFROM = "RINEX header data from ";
const string LOG_MSG_OBSFROM = "RINEX observation data from ";
const string LOG_MSG_NAVFROM = "RINEX navigation data from ";
const string LOG_MSG_GENOBS = "Generate RINEX observation files";
const string LOG_MSG_GENNAV = "Generate RINEX navigation files";
const string LOG_MSG_IGNF = "Ignored file: ";
const string LOG_MSG_FILNTS  = "File name too short: ";
const string LOG_MSG_PRCINF = "Process input file ";
const string LOG_MSG_PRCD = "Processed ";
const string LOG_MSG_EPOIN = " epochs in ";
const string LOG_MSG_PRCHFF = "Process header from file ";
const string LOG_MSG_OBSFNS = "Observation files not selected";
const string LOG_MSG_NAVFNS = "Navigation files not selected";
const string LOG_MSG_NAVVER = "Nav version is not 2.10 or 3.02";
const string LOG_MSG_INFILENOK = "Cannot open file ";
const string LOG_MSG_OUTFILENOK = "Cannot create file ";

//raw data file extensions
const string obsExt = ".ORD";
const string navExt = ".NRD";
//return data
const unsigned int RET_ERR_OPENRAW = 1;
const unsigned int RET_ERR_READRAW = 2;
const unsigned int RET_ERR_CREOBS = 4;
const unsigned int RET_ERR_WRIOBS = 8;
const unsigned int RET_ERR_CRENAV = 16;
const unsigned int RET_ERR_WRINAVM = 32;
const unsigned int RET_ERR_WRINAVG = 64;
const unsigned int RET_ERR_WRINAVR = 128;
const unsigned int RET_ERR_WRINAVS = 256;
const string RET_MSG_OK = "0; RINEX file(s) generated";
const string RET_MSG_ERR = "; RINEX file(s) errors ";

//defined below
int generateRINEXobs(vector<string> rnxPar, string inName, FILE* inFile, string rinexPath, Logger* plog);
void setHdData(RinexData &rinex, GNSSdataFromGRD &gnssRaw, Logger* plog, vector<string> rnxPar);
bool printNavFile(RinexData rinex, string inNavFileNamePrefix, string outfilesFullPath, Logger* plog);
string getErrMsg(unsigned int errCode);
/**
 * generateRinexFilesJNI is the interface routine with the Java application toRINEX.
 * It is called to generate RINEX files using data acquired from the GNSS receiver (which
 * are stored in raw data files) and parameters passed in arguments.
 * @param env with the app environment (Java specific)
 * @param me the interface object
 * @param infilesPath the full path to the directory where raw data files are placed
 * @param infilesName an array of strings each one containing the name of a raw data file
 * @param outfilesPath the full path to the directory where RINEX files will be generated
 * @param rinexParams an array of strings each one containing a pair of messaje_type;value 
 */
extern "C"
JNIEXPORT jstring JNICALL Java_com_gnssapps_acq_torinex_GenerateRinex_generateRinexFilesJNI(
        JNIEnv *env,
        jobject me, /* this */
        jint toDo,  /*0 = one RINEX per ORD; 1 = one RINEX for all ORD*/
        jstring infilesPath,
        jobjectArray infilesName,
        jstring outfilesPath,
        jobjectArray rinexParams) {
    //useful general vars
    int n;
    //get some params values
    const char* retValue;
    jstring jFileName;
    string pntName;
    string infilesFullPath = string(env->GetStringUTFChars(infilesPath, 0)) + "/";
    string outfilesFullPath = string(env->GetStringUTFChars(outfilesPath, 0)) + "/";
    /// 1- Defines and sets the error logger object
    Logger log(outfilesFullPath + LOG_FILENAME, string(), string(LOG_STARTMSG));
    /// 2 -extract rinex header data from params
    vector<string> vrinexParams;
    n = env->GetArrayLength(rinexParams);
    for (int i = 0; i < n; i++) {
        vrinexParams.push_back(string(env->GetStringUTFChars((jstring) (env->GetObjectArrayElement(rinexParams, i)), 0)));
    };
    /// 3 -classify observation and navigation input file
    vector<string> inObsFileNames;
    vector<string> inNavFileNames;
    for (int i = 0; i < env->GetArrayLength(infilesName); ++i) {
        pntName = string(env->GetStringUTFChars((jstring) (env->GetObjectArrayElement(infilesName, i)), 0));
        //check if it is observation file
        if (pntName.length() > obsExt.length()) {
            if (pntName.substr(pntName.length() - obsExt.length(), obsExt.length()).compare(obsExt) == 0) {
                inObsFileNames.push_back(pntName);
            } else if (pntName.substr(pntName.length() - navExt.length(), navExt.length()).compare(navExt) == 0) {
                inNavFileNames.push_back(pntName);
            } else log.info(LOG_MSG_IGNF + pntName);
        } else log.info(LOG_MSG_FILNTS + pntName);
    }
    /// 4 -create observation files
    RinexData rinex(RinexData::V210, &log);
    string outFileName;	//the output file name for RINEX files
    FILE* outFile;		//the RINEX file where data will be printed
    int epochCount, week, eventFlag;
    double tow, bias;
    unsigned int retError = 0;
    GNSSdataFromGRD gnssRaw(&log);
    string svoid = string();
    if (!inObsFileNames.empty()) {
        log.info(LOG_MSG_GENOBS);
        if((int) toDo == 0) {
            /// 4.1 -create one RINEX file for each ORD file
            for (int i = 0; i < inObsFileNames.size(); ++i) {
                //open input raw data file
                if (gnssRaw.openInputGRD(infilesFullPath, inObsFileNames[i])) {
                    log.info(LOG_MSG_PRCINF + infilesFullPath + inObsFileNames[i]);
                    setHdData(rinex, gnssRaw, &log, vrinexParams);   //set RINEX header records
                    //open output RINEX file
                    outFileName = rinex.getObsFileName("PT" + inObsFileNames[i].substr(5, 2));
                    if ((outFile = fopen((outfilesFullPath + outFileName).c_str(), "w")) != NULL) {
                        epochCount = 0;
                        try {   //print RINEX header and each existing epoch in the raw data file
                            log.info(LOG_MSG_OBSFROM + inObsFileNames[i]);
                            rinex.printObsHeader(outFile);
                            gnssRaw.rewindInputGRD();
                            while (gnssRaw.collectEpochObsData(rinex)) {
                                rinex.printObsEpoch(outFile);
                                epochCount++;
                            }
                            rinex.printObsEOF(outFile);
                        } catch (string error) {
                            log.severe(error);
                            retError |= RET_ERR_WRIOBS;
                        }
                        fclose(outFile);
                        log.info(LOG_MSG_PRCD + to_string(epochCount) + LOG_MSG_EPOIN + inObsFileNames[i]);
                    } else {
                        log.severe(LOG_MSG_OUTFILENOK + outFileName);
                        retError |= RET_ERR_CREOBS;
                    }
                    gnssRaw.closeInputGRD();
                } else {
                    log.warning(LOG_MSG_INFILENOK + inObsFileNames[i]);
                    retError |= RET_ERR_OPENRAW;
                }
            }
        } else {
            /// 4.2 -create a unique RINEX V3 file containing observation data from all raw data files
            //extract from the first input file RINEX header data
            if (gnssRaw.openInputGRD(infilesFullPath, inObsFileNames[0])) {
                log.info(LOG_MSG_PRCHFF + infilesFullPath + inObsFileNames[0]);
                setHdData(rinex, gnssRaw, &log, vrinexParams);   //set RINEX header records
                gnssRaw.closeInputGRD();
                //open output RINEX file
                outFileName = rinex.getObsFileName("PT" + inObsFileNames[0].substr(5, 2));
                if ((outFile = fopen((outfilesFullPath + outFileName).c_str(), "w")) != NULL) {
                    //print RINEX file header and iterate over existing input raw data files to print observation data
                    rinex.printObsHeader(outFile);
                    for (int i = 0; i < inObsFileNames.size(); ++i) {
                        if (gnssRaw.openInputGRD(infilesFullPath, inObsFileNames[i])) {
                            log.info(LOG_MSG_OBSFROM + infilesFullPath + inObsFileNames[i]);
                            epochCount = 0;
                            try {
                                //one epoch special event 3 shall be added between observation data from different raw data files
                                if (i != 0) {
                                    //clean header records and set the ones used to print an epoch special event 3 (new site occupation)
                                    rinex.clearHeaderData();
                                    rinex.setHdLnData(rinex.MRKNAME, inObsFileNames[i], svoid, svoid);
                                    rinex.getEpochTime(week, tow, bias, eventFlag);
                                    rinex.setEpochTime(week, tow, bias, 3);
                                    rinex.printObsEpoch(outFile);
                                }
                                //for each existing epoch in the input raw data file, print it
                                while (gnssRaw.collectEpochObsData(rinex)) {
                                    rinex.printObsEpoch(outFile);
                                    epochCount++;
                                }
                            } catch (string error) {
                                log.severe(error);
                                retError |= RET_ERR_WRIOBS;
                            }
                            gnssRaw.closeInputGRD();
                            log.info(LOG_MSG_PRCD + to_string(epochCount) + LOG_MSG_EPOIN + inObsFileNames[i]);
                        } else {
                            log.warning(LOG_MSG_INFILENOK + inObsFileNames[i]);
                            retError |= RET_ERR_OPENRAW;
                        }
                    }
                    rinex.printObsEOF(outFile);
                    fclose(outFile);
                } else {
                    log.severe(LOG_MSG_OUTFILENOK + outFileName);
                    retError |= RET_ERR_CREOBS;
                }
            } else {
                log.severe(LOG_MSG_INFILENOK + inObsFileNames[0]);
                retError |= RET_ERR_OPENRAW;
            }
        }
    } else log.info(LOG_MSG_OBSFNS);
    //5 -create navigation file(s). Here toDo value does not apply: RINEX navigation files are created
    /// depending on the version requested and existence of navigation data from one or several constellations.
    /// If version is V3.01, one navigation RINEX file can be generated with data from several constellations.
    /// If version to print is V2.10 one navigation RINEX file would be generated for each constellation having data.
    if (!inNavFileNames.empty()) {
        log.info(LOG_MSG_GENNAV);
        /// 5.1 -get navigationn data from NRD files
        for (int i = 0; i < inNavFileNames.size(); ++i) {
            //open input raw data file
            if (gnssRaw.openInputGRD(infilesFullPath, inNavFileNames[i])) {
                log.info(LOG_MSG_PRCHFF + infilesFullPath + inNavFileNames[i]);
                setHdData(rinex, gnssRaw, &log, vrinexParams);   //set RINEX header records
                gnssRaw.rewindInputGRD();
                if (gnssRaw.collectNavData(rinex)) {
                    log.info(LOG_MSG_PRCD + inNavFileNames[i]);
                } else {
                    log.warning(LOG_MSG_INFILENOK + inNavFileNames[i]);
                    retError |= RET_ERR_READRAW;
                }
                gnssRaw.closeInputGRD();
            } else {
                log.warning(LOG_MSG_INFILENOK + inNavFileNames[i]);
                retError |= RET_ERR_OPENRAW;
            }
        }
        /// 5.2 -create RINEX files containing navigation data from raw data files
        double ver;
        char sysId;
        char fileType;
        rinex.getHdLnData(rinex.VERSION, ver, fileType, sysId);
        //TBD selsystem?
        if (ver == 2.10) {
            ///If version is V2.10, a navigation RINEX will be created for each constellation
            ///but constellation can be GPS, GLONASS or SBAS
            char constNav[] = {'G', 'R', 'S'};
            for (int i = 0; i < 3; ++i) {
                if (rinex.hasNavEpochs(constNav[i])) {
                    rinex.setHdLnData(rinex.VERSION, ver, 'N', constNav[i]);
                    if(!printNavFile(rinex, "PT" + inNavFileNames[0].substr(5, 2), outfilesFullPath, &log))
                        retError |= RET_ERR_WRINAVG<<i;
                }
            }
        } else if (ver == 3.02) {
            /// If version is V3.02, a unique navigation RINEX will containt all nav data
            rinex.setHdLnData(rinex.VERSION, ver, 'N', 'M');
            if (!printNavFile(rinex, "PT" + inNavFileNames[0].substr(5, 2), outfilesFullPath, &log))
                retError |= RET_ERR_WRINAVM;
        } else {
            log.severe(LOG_MSG_NAVVER);
            retError |= RET_ERR_OPENRAW;
        }
    } else log.info(LOG_MSG_NAVFNS);
    return env->NewStringUTF(getErrMsg(retError).c_str());
}
/**
 * setHdData sets RINEX header records extracting data from the parameters passed and from
 * the the message types containing header data in the given input file.
 * @param rinex the rinex object where header records are stored
 * @param rnxPar the RINEX setup parameters with data for the header
 * @param inFile the input raw data file containing messages with header data
 * @param plog a pointer to the logger
 */
//void setHdData(RinexData &rinex, vector<string> rnxPar, FILE* inFile, Logger* plog) {
void setHdData(RinexData &rinex, GNSSdataFromGRD &gnssRaw, Logger* plog, vector<string> rnxPar) {
    //GNSSdataFromGRD gnssRaw(inFile, plog);
    int msgType;
    string msgContent;
    size_t pos;
    try {
        //set some RINEX header records with default values
        rinex.setHdLnData(RinexData::ANTTYPE, string("1"), string("Unknown"));
        rinex.setHdLnData(RinexData::ANTHEN, (double) 0.0, (double) 0.0, (double) 0.0);
        rinex.setHdLnData(RinexData::TOFO, string("GPS"));
        rinex.setHdLnData(RinexData::WVLEN, (int) 1, (int) 0);
        //set RINEX header records from data in setup parameters
        plog->info(LOG_MSG_HDFROM + "setup");
        for (vector<string>::iterator it = rnxPar.begin(); it != rnxPar.end(); ++it) {
            msgType = stoi(*it, &pos);
            msgContent = (*it).substr(++pos);
            gnssRaw.processHdData(rinex, msgType, msgContent);
        }
    } catch (string error) {
        plog->severe(error);
    }
    //set RINEX header records from data in raw data file
    plog->info(LOG_MSG_HDFROM + "from file");
    if(!gnssRaw.collectHeaderData(rinex)) {
        plog->warning("All, or some header data not acquired");
    }
    gnssRaw.processFilterData(rinex);
}

/**
 * printNavFile prints the navigation RINEX file using data passed in the current RinexData object.
 * File is named and printed taking into account values stated in the RINEX VERSION / TYPE record.
 * @param rinex the object containing data and parameters to be used for printing
 * @param inNavFileNamePrefix the prefix to use to generate the navigation file name
 * @param outfilesFullPath the directory name (full path) where file will be generated
 * @param plog the logger to record log messages
 * @return true if file was generated, false otherwise
 */
bool printNavFile(RinexData rinex, string inNavFileNamePrefix, string outfilesFullPath, Logger* plog) {
    const string LOG_MSG_NAVPRINTED("Generated RINEX nav file=");
    const string LOG_MSG_NAVERROR("Error when generating RINEX nav file=");
    FILE* outFile;
    string outFileName = rinex.getNavFileName(inNavFileNamePrefix);
    if ((outFile = fopen((outfilesFullPath + outFileName).c_str(), "w")) != NULL) {
        try {
            rinex.printNavHeader(outFile);
            rinex.printNavEpochs(outFile);
        } catch (string error) {
            plog->warning(LOG_MSG_NAVERROR + outFileName + " "+ error);
            fclose(outFile);
            return false;
        }
        plog->info(LOG_MSG_NAVPRINTED + outFileName);
        fclose(outFile);
        return true;
    }
    plog->warning(LOG_MSG_OUTFILENOK + outFileName);
    return false;
}

/**
 * getErrMsg gets a descriptive text for the srror code passed
 * @param errCode error code
 * @return a string with the error message
 */
string getErrMsg(unsigned int errCode) {
    string retMsg = RET_MSG_OK;
    if (errCode != 0) {
        retMsg = to_string(errCode) + RET_MSG_ERR;
    }
    return retMsg;
}
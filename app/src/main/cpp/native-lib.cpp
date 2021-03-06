/**
 * native-lib.ccp contains the interface routine to be called from Java to collect
 * data from raw data files (.ORD for Observation Raw Data, and .NRD for Navigation Raw Data)
 * and generate the related RINEX files.
 *
 *Copyright 2015, 2021 by Francisco Cancillo & Luis Cancillo
 *<p>
 *This file is part of the toRINEX APP.
 *<p>Ver.	|Date	|Reason for change
 *<p>---------------------------------
 *<p>V1.0	|6/2018	|First release, not fully tested due to lack of device providing nav data
 *<p>V1.1   |6/2019 |Changes have been introduced correct errors in generation of a RINEX file
 *                  |from multiple input raw data files, o when generating multiple files.
 *                  |Also the processing order has been changed: first navigation files will be
 *                  |processed to allow extraction of navigation data usefull for observations
 *                  |processing, and them observation files are generated.
 */
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
const string LOG_MSG_NAVFROM = "RINEX navigation data from file";
const string LOG_MSG_GENOBS = "Generate RINEX observation files";
const string LOG_MSG_GENNAV = "Generate RINEX navigation files";
const string LOG_MSG_IGNF = "Ignored file: ";
const string LOG_MSG_FILNTS  = "File name too short: ";
const string LOG_MSG_PRCINF = "Process input file ";
const string LOG_MSG_PRCD = "Processed ";
const string LOG_MSG_EPOIN = " epochs in ";
const string LOG_MSG_PRCHFF = "Extract header records from file ";
const string LOG_MSG_OBSFNS = "Observation files not selected";
const string LOG_MSG_NAVFNS = "Navigation files not selected";
const string LOG_MSG_NAVVER = "Nav version set to 2.10 by default";
const string LOG_MSG_INFILENOK = "Cannot open file ";
const string LOG_MSG_OUTFILENOK = "Cannot create file ";
const string LOG_MSG_LITE = "Function not implemented in This LITE version";
const string MSG_NEW_SITE = "         --> THIS IS THE START OF A NEW SITE <--";
const string MSG_SRC_FILE = "Source file: ";
const string MSG_SRC_DIR  = "Source dir.: ";
//raw data file extensions
const string obsExt = ".ORD";
const string navExt = ".NRD";
//return codes
const unsigned int RET_ERR_OPENRAW = 1;
const unsigned int RET_ERR_READRAW = 2;
const unsigned int RET_ERR_CREOBS = 4;
const unsigned int RET_ERR_WRIOBS = 8;
const unsigned int RET_ERR_CRENAV = 16;
const unsigned int RET_ERR_WRINAV = 32;
//defined below
bool extractRinexHeaderData(RinexData* prinex, GNSSdataFromGRD* pgnssRaw, Logger* plog, vector<string> rnxPar, int inFileNum = 0, int inFileLast = 0);
unsigned int printNavFiles(RinexData* prinex,  Logger* plog, string outfilesFullPath, string markName);
unsigned int printOneNavFile(RinexData* prinex, Logger* plog, vector<string> selSys, string outfilesFullPath, string markName);
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
 * @return a text string describing the processing result for generating RINEX files
 */
extern "C"
JNIEXPORT jstring JNICALL Java_com_gnssapps_acq_torinex_GenerateRinex_generateRinexFilesJNI(
        JNIEnv *env,
        jobject me, /* this */
        jint filesToPrint,  /*0 = one RINEX per ORD; 1 = one RINEX for all ORD*/
        jstring infilesPath,
        jobjectArray infilesName,
        jstring outfilesPath,
        jobjectArray rinexParams) {
    //multipurpose general vars
    int n;
    string s;
    //get some params values
    string infilesFullPath = string(env->GetStringUTFChars(infilesPath, 0));
    string survey = infilesFullPath.substr(infilesFullPath.rfind('/') + 1);
    infilesFullPath += "/";
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
        s = string(env->GetStringUTFChars((jstring) (env->GetObjectArrayElement(infilesName, i)), 0));
        //check if it is observation file
        if (s.length() > obsExt.length()) {
            if (s.substr(s.length() - obsExt.length(), obsExt.length()).compare(obsExt) == 0) {
                inObsFileNames.push_back(s);
            } else if (s.substr(s.length() - navExt.length(), navExt.length()).compare(navExt) == 0) {
                inNavFileNames.push_back(s);
            } else log.info(LOG_MSG_IGNF + s);
        } else log.info(LOG_MSG_FILNTS + s);
    }
    string markName;    //to be used tu construct the open file name and set this RINEX header line
    string outFileName;	//the output file name for RINEX files
    FILE* outFile;		//the RINEX file where data will be printed
    int epochCount, week, eventFlag;
    double tow, bias;
    bool dataAvailable;
    GNSSdataFromGRD* pgnssRaw = new GNSSdataFromGRD(&log);
    RinexData* prinex;
    unsigned int retError = 0;
    /// 4 -create navigation file(s)
    // Here filesToPrint value does not apply: RINEX navigation files are created depending on the version requested
    // and navigation data availability from one or several constellations.
    // If version is V3, one navigation RINEX file can be generated with data from several constellations.
    // If version to print is V2 one navigation RINEX file would be generated for each constellation having data.
    // Note that V2.10 defines only formats for GPS, GLONASS and SBAS
    if (!inNavFileNames.empty()) {
        log.info(LOG_MSG_GENNAV);
        /// 4.1 -extract from raw data files header and navigation data
        prinex = new RinexData(RinexData::V210, &log);
        //extract from input files RINEX header and navigation data
        n = inNavFileNames.size();
        dataAvailable = false;
        for (int i = 0; i < n; ++i) {
            if (pgnssRaw->openInputGRD(infilesFullPath, inNavFileNames[i])) {
                log.info(LOG_MSG_PRCINF + infilesFullPath + inNavFileNames[i]);
                if (extractRinexHeaderData(prinex, pgnssRaw, &log, vrinexParams, i, n - 1)) {
                    dataAvailable = true;
                    prinex->setHdLnData(RinexData::COMM, RinexData::COMM, MSG_SRC_FILE + inNavFileNames[i]);
                    pgnssRaw->rewindInputGRD();
                    log.info(LOG_MSG_NAVFROM);
                    pgnssRaw->collectNavData(*prinex);
                }
                pgnssRaw->closeInputGRD();
            } else {
                log.severe(LOG_MSG_INFILENOK + inNavFileNames[i]);
                retError |= RET_ERR_OPENRAW;
            }
        }
        if (dataAvailable) {
            prinex->getHdLnData(RinexData::MRKNAME, markName);
            if (markName.length() == 0) markName = inNavFileNames[0].substr(0, inNavFileNames[0].find('.'));
            retError |= printNavFiles(prinex, &log, outfilesFullPath, markName);
        } else retError |= RET_ERR_READRAW;
        delete prinex;
    } else log.info(LOG_MSG_NAVFNS);
    /// 5 -create observation files
    if (!inObsFileNames.empty()) {
        s = string();
        log.info(LOG_MSG_GENOBS);
        if((int) filesToPrint == 0) {
            /// 5.1 -create one RINEX file for each ORD file
            for (int i = 0; i < inObsFileNames.size(); ++i) {
                prinex = new RinexData(RinexData::V210, &log);
                //open input raw data file
                if (pgnssRaw->openInputGRD(infilesFullPath, inObsFileNames[i])) {
                    log.info(LOG_MSG_PRCINF + infilesFullPath + inObsFileNames[i]);
                    if (extractRinexHeaderData(prinex, pgnssRaw, &log, vrinexParams)) {
                        //input file can be processed
                        //open output RINEX file
                        prinex->getHdLnData(RinexData::MRKNAME, markName);
                        if (markName.length() == 0) markName = inObsFileNames[i].substr(0, inObsFileNames[i].find('.'));
                        outFileName = prinex->getObsFileName(markName);
                        if ((outFile = fopen((outfilesFullPath + outFileName).c_str(), "w")) != NULL) {
                            epochCount = 0;
                            try {   //print RINEX header and each existing epoch in the raw data file
                                log.info(LOG_MSG_OBSFROM + inObsFileNames[i]);
                                prinex->setHdLnData(RinexData::COMM, RinexData::COMM, MSG_SRC_FILE + inObsFileNames[i]);
                                prinex->setHdLnData(RinexData::COMM, RinexData::COMM, MSG_SRC_DIR + survey);
                                prinex->setHdLnData(RinexData::MRKNAME, markName, s, s);
                                prinex->printObsHeader(outFile);
                                pgnssRaw->rewindInputGRD();
                                while (pgnssRaw->collectEpochObsData(*prinex)) {
                                    prinex->printObsEpoch(outFile);
                                    epochCount++;
                                }
                                prinex->printObsEOF(outFile);
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
                        pgnssRaw->closeInputGRD();
                    }
                } else {
                    log.warning(LOG_MSG_INFILENOK + inObsFileNames[i]);
                    retError |= RET_ERR_OPENRAW;
                }
                delete prinex;
            }
        } else {
            /// 5.2 -create a unique RINEX file containing observation data from all raw data files
            prinex = new RinexData(RinexData::V210, &log);
            //extract from input files RINEX header data
            n = inObsFileNames.size();
            dataAvailable = false;
            for (int i = 0; i < n; ++i) {
                if (pgnssRaw->openInputGRD(infilesFullPath, inObsFileNames[i])) {
                    log.info(LOG_MSG_PRCHFF + infilesFullPath + inObsFileNames[i]);
                    if (extractRinexHeaderData(prinex, pgnssRaw, &log, vrinexParams, i, n - 1)) {
                        prinex->setHdLnData(RinexData::COMM, RinexData::COMM, MSG_SRC_FILE + inObsFileNames[i]);
                        dataAvailable = true;
                    }
                    else inObsFileNames[i].clear();
                    pgnssRaw->closeInputGRD();
                }
            }
            prinex->getHdLnData(RinexData::MRKNAME, markName);
            if (markName.length() == 0) markName = inObsFileNames[0].substr(0, inObsFileNames[0].find('.'));
            prinex->setHdLnData(RinexData::COMM, RinexData::COMM, MSG_SRC_DIR + survey);
            prinex->setHdLnData(RinexData::MRKNAME, markName, s, s);
            //open output RINEX file
            outFileName = prinex->getObsFileName(markName);
            if (dataAvailable) {
                if ((outFile = fopen((outfilesFullPath + outFileName).c_str(), "w")) != NULL) {
                    //print RINEX file header and iterate over existing input raw data files to print observation data
                    prinex->printObsHeader(outFile);
                    for (int i = 0; i < inObsFileNames.size(); ++i) {
                        if (!inObsFileNames[i].empty()) {
                            if (pgnssRaw->openInputGRD(infilesFullPath, inObsFileNames[i])) {
                                log.info(LOG_MSG_OBSFROM + infilesFullPath + inObsFileNames[i]);
                                epochCount = 0;
                                try {
                                    //one epoch special event 3 shall be added between observation data from different raw data files
                                    if (i != 0) {
                                        markName = inObsFileNames[i].substr(0, inObsFileNames[i].find('.'));
                                        //clean header records and set the ones used to print an epoch special event 3 (new site occupation)
                                        prinex->clearHeaderData();
                                        prinex->setHdLnData(RinexData::COMM, RinexData::COMM, MSG_NEW_SITE);
                                        prinex->setHdLnData(RinexData::MRKNAME, markName, s, s);
                                        prinex->getEpochTime(week, tow, bias, eventFlag);
                                        prinex->setEpochTime(week, tow, bias, 3);
                                        prinex->printObsEpoch(outFile);
                                    }
                                    //for each existing epoch in the input raw data file, print it
                                    while (pgnssRaw->collectEpochObsData(*prinex)) {
                                        prinex->printObsEpoch(outFile);
                                        epochCount++;
                                    }
                                } catch (string error) {
                                    log.severe(error);
                                    retError |= RET_ERR_WRIOBS;
                                }
                                pgnssRaw->closeInputGRD();
                                log.info(LOG_MSG_PRCD + to_string(epochCount) + LOG_MSG_EPOIN + inObsFileNames[i]);
                            } else {
                                log.warning(LOG_MSG_INFILENOK + inObsFileNames[i]);
                                retError |= RET_ERR_OPENRAW;
                            }
                        }
                    }
                    prinex->printObsEOF(outFile);
                    fclose(outFile);
                } else {
                    log.severe(LOG_MSG_OUTFILENOK + outFileName);
                    retError |= RET_ERR_CREOBS;
                }
            } else retError |= RET_ERR_READRAW;
            delete prinex;
        }
    } else log.info(LOG_MSG_OBSFNS);
    delete pgnssRaw;
    return env->NewStringUTF(to_string(retError).c_str());
}
/**
 * extractRinexHeaderData sets RINEX header records extracting data from the parameters passed and from
 * the the message types containing header data in the given input file.
 * @param prinex pointer to the rinex object where header records will be stored
 * @param pgnssRaw pointer to the GNSS raw data object with data to be extracted
 * @param plog a pointer to the logger
 * @param rnxPar the vector with the list of Rinex data passed as paramenters
 * @param inFileNum the number of the current input raw data file. First value = 0
 * @param inFileLast the last number of the input files to be processed
 * @return true when header data have been extracted from input files, false otherwise
 */
bool extractRinexHeaderData(RinexData* prinex, GNSSdataFromGRD* pgnssRaw, Logger* plog, vector<string> rnxPar, int inFileNum, int inFileLast) {
    //GNSSdataFromGRD gnssRaw(inFile, plog);
    int msgType;
    string msgContent;
    size_t pos;
    //set some RINEX header records with default values, but only for the first file
    if (inFileNum == 0)  {
        try {
            prinex->setHdLnData(RinexData::ANTTYPE, string("1"), string("Unknown"));
            prinex->setHdLnData(RinexData::ANTHEN, (double) 0.0, (double) 0.0, (double) 0.0);
            prinex->setHdLnData(RinexData::WVLEN, (int) 1, (int) 0);
            //set RINEX header records from data in setup parameters
            plog->info(LOG_MSG_HDFROM + "setup");
            for (vector<string>::iterator it = rnxPar.begin(); it != rnxPar.end(); ++it) {
                msgType = stoi(*it, &pos);
                msgContent = (*it).substr(++pos);
                pgnssRaw->processHdData(*prinex, msgType, msgContent);
            }
        } catch (string error) {
            plog->severe(error);
        }
    }
    //set RINEX header records from data in raw data file
    plog->info(LOG_MSG_HDFROM + "file");
    return pgnssRaw->collectHeaderData(*prinex, inFileNum, inFileLast);
}
/**
 * printNavFiles conducts the printing of the RINEX navigation file or files for the navigation
 * data already collected from the raw data files.
 * Note that depending on the RINEX version to be printed it should be printed a unique file, if
 * version 3 requested, or one file for each constellation, if version 2 requested.
 *
 * @param prinex pointer to the rinex object where header records will be stored
 * @param plog a pointer to the logger
 * @param outfilesFullPath is the path name where the output file will be created
 * @param markName the position mark name to be used to name the output file
 * @return 0 if no error occurred, or a code (in the set RET_ERR_xxxx values) identifying the error
 */
unsigned int printNavFiles(RinexData* prinex,  Logger* plog, string outfilesFullPath, string markName) {
    double rinexVersion;
    char rinexType;
    char constellationToPrint;
    vector<string> selSys;
    vector<string> selObs;
    unsigned int retCode = 0;
    try {
        //set RINEX version to be printed
        prinex->getHdLnData(RinexData::VERSION, rinexVersion, rinexType, constellationToPrint);
        if (rinexVersion < 2.0) {
            //should never happen
            plog->warning(LOG_MSG_NAVVER);
            rinexVersion = 2.10;
            prinex->setHdLnData(RinexData::VERSION, rinexVersion);
        }
        if (rinexVersion < 3.0) {
            //print a RINEX file for each existing constellation
            for (unsigned int i=0; prinex->getHdLnData(RinexData::SYS, constellationToPrint, selObs, i); i++) {
                selSys.clear();
                selObs.clear();
                selSys.push_back(string(1, constellationToPrint));
                retCode = retCode | printOneNavFile(prinex, plog, selSys, outfilesFullPath, markName);
            }
        } else {
            //print one RINEX file for all constellations
            retCode = printOneNavFile(prinex, plog, selSys, outfilesFullPath, markName);
        }
    } catch (string error) {
        plog->severe(error);
    }
    return retCode;
}
/**
 * printOneNavFile print one navigaation file for the selected systems in the path passed in arguments.
 * It is assumed coherence between the systems to be included and the RINEX version to be printed.
 *
 * @param prinex pointer to the rinex object where header records will be stored
 * @param plog a pointer to the logger
 * @param selSys with the list of systems to be included in the RINEX file. If empty all systems will be printed
 * @param outfilesFullPath is the path name where the output file will be created
 * @param markName the position mark name to be used to name the output file
 * @return 0 if no error occurred, or a code (in the set RET_ERR_xxxx values) identifying the error
 */
unsigned int printOneNavFile(RinexData* prinex, Logger* plog, vector<string> selSys, string outfilesFullPath, string markName) {
    vector<string> emptyVector;
    string outFileName;	//the output file name for RINEX files
    FILE* outFile;		//the RINEX file where data will be printed
    unsigned int retValue = 0;
    prinex->setFilter(selSys, emptyVector);
    //if (selSys.size() != 0) markName = selSys[0] + markName;
    outFileName = prinex->getNavFileName(markName);
    if ((outFile = fopen((outfilesFullPath + outFileName).c_str(), "w")) != NULL) {
        try {
            prinex->printNavHeader(outFile);
            prinex->printNavEpochs(outFile);
        } catch (string error) {
            plog->severe(error);
            retValue = RET_ERR_WRINAV;
        }
        fclose(outFile);
    }
    else retValue = RET_ERR_CRENAV;
    return retValue;
}
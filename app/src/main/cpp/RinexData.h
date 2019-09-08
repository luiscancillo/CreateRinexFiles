/** @file RinexData.h
 * Contains RinexData class definition.
 * A RinexData object contains data for the header and epochs of the RINEX files.
 * Methods are provided to store data into RinexData objects, print them to RINEX files,
 * and to perform the reverse operation: read RINEX files data and store them into RinexData objects.
 *
 *Copyright 2015 Francisco Cancillo
 *<p>
 *This file is part of the RXtoRINEX tool.
 *<p>
 *RXtoRINEX is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 *as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *RXtoRINEX is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *<p>
 *A copy of the GNU General Public License can be found at <http://www.gnu.org/licenses/>.
 *<p>Ver.	|Date	|Reason for change
 *<p>---------------------------------
 *<p>V1.0	|2/2015	|First release
 *<p>V2.0	|6/2016	|Added functionality:
 *<p>				|-#	For setting values to header records, and observation and navigation epochs.
 *<p>				|-#	For reading RINEX files.
 *<p>				|-#	For getting data current values from header records and from observation and navigation epochs (useful after reading files).
 *<p>				|-#	For filtering observation and navigation epoch data according to selected systems/satellites/observables (useful before printing or getting data).
 *<p>				|Removed functionalities not related to RINEX file processing.
 *<p>V2.1	|2/2018	|Reviewed to run on Linux / Android.
 *<p>               |Added msg variables
 *<p>V2.2   |6/2019 |Simplified the processing of signal names for V2 RINEX
 *                  |Simplified the processing of systems and signals using new data structure for systems
 */
#ifndef RINEXDATA_H
#define RINEXDATA_H

#include <vector>
#include <string>
#include <algorithm>

#include "Logger.h"	//from CommonClasses

using namespace std;

//@cond DUMMY
//maximum number of lines and columns in the RINEX broadcast orbit array
#define BO_MAXLINS 12
#define BO_MAXCOLS 4
#define BO_IONOA_LIN BO_MAXLINS-4   //the line index for ionospheric corrections
#define BO_IONOB_LIN BO_MAXLINS-3   //the line index for ionospheric corrections
#define BO_TIME_LIN BO_MAXLINS-2   //the line index for time systems corrections
#define BO_LEAP_LIN BO_MAXLINS-1  //the line index for leap seconds corrections
//Number of Broadcast Orbit lines for Glonass
#define BO_MAXLINS_GLO 4
//Number of Broadcast Orbit lines for GPS
#define BO_MAXLINS_GPS 8
//Number of Broadcast Orbit lines for Galileo
#define BO_MAXLINS_GAL 8
//Number of Broadcast Orbit lines for Beidou
#define BO_MAXLINS_BDS 8
//Number of Broadcast Orbit lines for SBAS
#define BO_MAXLINS_SBAS 4
//Number of Broadcast Orbit lines for QZSS
#define BO_MAXLINS_QZSS 4
//Other related constants
const string IONO_GAL_DES("GAL");
const string IONO_GPSA_DES("GPSA");
const string IONO_GPSB_DES("GPSB");
const string IONO_QZSA_DES("QZSA");
const string IONO_QZSB_DES("QZSB");
const string IONO_BDSA_DES("BDSA");
const string IONO_BDSB_DES("BDSB");

const double MAXOBSVAL = 9999999999.999; //the maximum value for any observable to fit the F14.4 RINEX format
const double MINOBSVAL = -999999999.999; //the minimum value for any observable to fit the F14.4 RINEX format
//Mask values to define RINEX header record/label type
const unsigned int NAP = 0x00;		//Not applicable for the given file type
const unsigned int OBL = 0x01;		//Obligatory
const unsigned int OPT = 0x02;		//Optional
const unsigned int MSK = 0x03;		//The mask to apply for filtering these bits
const unsigned int OBSNAP = NAP;		//Not applicable for the OBS file type
const unsigned int OBSOBL = OBL;		//Obligatory
const unsigned int OBSOPT = OPT;		//Optional
const unsigned int OBSMSK = MSK;	//The mask to apply to filter these bits
const unsigned int NAVNAP = NAP<<2;		//Not applicable for the navigation file type
const unsigned int NAVOBL = OBL<<2;		//Obligatory
const unsigned int NAVOPT = OPT<<2;		//Optional
const unsigned int NAVMSK = MSK<<2;	//The mask to apply to filter these bits
//observation values in RINEX V3 having equivalent in RINEX V2. Shall have the same size!
const string v3obsTypes[] = {"C1C", "L1C", "D1C", "S1C", "C1P", "C2P", "L2P", "D2P", "S2P", string()};
const string v2obsTypes[] = {"C1" , "L1" , "D1" , "S1" , "P1" , "P2" , "L2" , "D2" , "S2" , string()};
//Messages common to several methods
const string msgSpace(" ");
const string msgComma(",");
const string msgSlash("/");
const string msgColon(": ");
const string msgBrak("]");
const string msgEpoch("Epoch [");
const string msgNoFlag(" Missed flag.");
const string msgGetHdLn(" (getHdLnData)");
const string msgHdRecNoData(" is obligatory, but has not data");
const string msgNotInSYS("NOT in SYS/TOBS records");
const string msgNotSys("Satellite systems not defined or none selected");
const string msgSatOrSp(" Missed number of sats or special records.");
const string msgSetHdLn(" (setHdLnData)");
const string msgSysUnk("Satellite system code unknown=");
const string msgUnexpObsEOF("Unexpected EOF in observation record");
const string msgVerTBD("Undefined version to print");
const string msgWrongDate("Wrong date-time");
const string msgWrongFlag(" Wrong flag");
const string msgWrongPRN("Wrong PRN");
const string msgNoLabel("No header label found in ");
const string msgWrongLabel(" cannot be used in this RINEX version");
const string msgProcessV210("File processed as per V2.1");
const string msgProcessV301("File processed as per 3.01");
const string msgProcessTBD("Cannot cope with this input file version. TBD assumed");
const string msgNumsat7(" Number of sats >=7");
const string msgTransit("Cannot cope with Transit data");
const string msgWrongFormat("Wrong data format in this line. ");
const string msgObsNoTrans(" Observable type cannot be traslated to V302");
const string msgMisCode("Mismatch in number of expected and existing code types");
const string msgNumTypesNo("Number of observation types not specified");
const string msgTypes(" types");
const string msgNoScale(" Scale factor not specified");
const string msgNoCorrection(" Correction not specified");
const string msgNoFreq(" no frequency number");
const string msgSlots(" slots");
const string msgNoSlot( " no slot number");
const string msgMisSlots("Mismatch in number of expected and existing slots");
const string msgWrongCont(" Continuation line not following a regular one");
const string msgInternalErr("Internal error: invalid label Id in readHdLineData");
const string msgFound("found");
const string msgDataRead(" data read");
const string msgErrIono(" errors in iono corrections:");
const string msgContExp("continuation expected, but received ");
const string msgFmtCont("wrong format in continuation line");
const string msgPhPerType(" phase shift correction, for signal and sats ");
const string msgErrBO("Error Broad.Orb.[");
const string msgWrongSysPRN("Wrong system or PRN");
const string msgWrongInFile("Wrong input file version");
const string msgNewEp("New epoch.");
const string msgStored("Stored.");
const string msgKinemEvent("Kinematic event: error in special records");
const string msgOccuEvent("New site occupation event: error in special records");
const string msgOccuEventNoMark("New site occupation event without MARKER NAME");
const string msgHdEvent("Header information event: error in special records");
const string msgExtEvent("External event without date");
const string msgIgnObservable("Ignored observable in epoch, satellite, observable=");
const string msgEpheSat("Ephemeris for sat ");
const string msgTimeTag(" time tag ");
const string msgAlrEx(". ALREADY EXIST");
const string msgSaved(". SAVED");

const string errorLabelMis("Internal error. Wrong argument types in RINEX label identifier=");
//@endcond

/**RinexData class defines a data container for the RINEX file header records, epoch observables, and satellite navigation ephemeris.
 *<p>The class provides methods to store RINEX data and set parameters in the container, and to print RINEX files.
 *<p>Also it includes methods to read existing RINEX files, to store their data in the container, and to access them.
 *<p>A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange
 * Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. Also an updated document exists
 * for Version 3.01.
 *<p>The usual way to use this class for generating a RINEX observation file would be:
 * -# Create a RinexData object stating at least the version to be generated.
 * -# Set data for the RINEX header records. Note that some records are obligatory, and therefore shall have data before printing the header.
 *	Data to header records can be provided directly using the setHdLnData methods.
 * -# Print the header of the RINEX file to be generated using the printObsHeader method.
 * -# Set observation data for the epoch to be printed using setEpochTime first and saveObsData repeatedly for each system/satellite/observable for this epoch.
 * -# Print the RINEX epoch data using the printObsEpoch method.
 * -# Repeat former steps 4 & 5 while epoch data exist.
 *<p>Alternatively input data can be obtained from another RINEX observation file. In this case:
 * - The method readRinexHeader is used in step 2 to read from another RINEX file header records data and store them into the RinexData object.
 * - The method readObsEpoch is used in step 4 to read an epoch data from another RINEX observation file.
 *<p>When it is necessary to print a special event epoch in the epochs processing cycle, already existing header records data shall be cleared before processing
 *any special event epoch having header records, that is, special events having flag values 2, 3, 4 or 5. The reason is that when printing such events,
 *after the epoch line they are printed all header line records having data. In sumary, to process a special event it will be encessary to perform
 *the following steps:
 * -# clearHeaderData before setting any setting of header record data. It is assumed that before doing this step RINEX headers have been printed.
 * -# setHdLnData for all records to be included in the special event.
 * -# setEpochTime stating the event flag value for the epoch to be printed.
 * -# printObsEpoch to print RINEX epoch data.
 *<p>To generate navigation files the process would be as per observation files, with the following steps:
 * -# Create a RinexData object stating at least the version to be generated.
 * -# Depending on version to be generated, it could be necessary to select using setFilter the unique system whose navigation data will be printed.
 * -# Set data for the RINEX header records as per above.
 * -# Print the header of the RINEX file to be generated using the printNavHeader method.
 * -# Set navigation data for the epoch to be printed using setEpochTime first and saveNavData repeatedly for each system/satellite for this epoch
 * -# Print the RINEX epoch data using the printNavEpoch method.
 * -# Repeat steps 5 & 6 while epoch data exist.
 *<p>As per above case, input data can be obtained from another RINEX navigation file. In this case:
 * - The method readRinexHeader is used in step 3 to read from another RINEX file header records data and store them into the RinexData object.
 * - The method readNavEpoch is used in step 5 to read an epoch data from another RINEX navigation file.
 *<p>Note that in RINEX V3.01 a navigation file can include ephemeris from several navigation systems, but in V2.10 a navigation file can include
 *data for only one system. This is the reason to provide to the class data on the system to be printed using the setFilter method, when
 * a V2.10 navigation file would be generated. 
 *<p>This class can be used to obtain data from existing RINEX observation files, the usual process would be:
 * -# Create a RinexData object
 * -# Use method readRinexHeader to read from the existing input RINEX file header records data. They are stored into the RinexData object.
 * -# Get needed data from the RINEX header records read using getHdLnData methods.
 * -# Use method readObsEpoch to read an epoch data from the input RINEX observation file.
 * -# Get needed observation data from this epoch using getObsData
 * -# Repeat former two steps while epoch data exist.
 *<p>To obtain satellite ephemeris data from RINEX navigation files the process would be similar:
 * -# Create a RinexData object
 * -# Use method readRinexHeader to read from the input RINEX file header records data and store them into the RinexData object.
 * -# Get needed data from the RINEX header records read using getHdLnData methods.
 * -# Use method readNavEpoch to read a satellite epoch data from the input RINEX navigation file.
 * -# Get needed navigation data from this epoch using getNavData
 * -# Repeat former two steps while epoch data exist.
 *<p>Finally, the class provides the possibility to filter observation or navigation data stored into a class object using methods to:
 * - Set the filtering criteria (select an epoch time period, a system/satellite/observation) using the setFilter method
 * - Discard from saved data these not belonging to the selected time period or systems/satellites/observations using the filterObsData or filterNavData.
 *<p>This class uses the Logger class defined also in this package.
 */
class RinexData {
public:
	/// RINEX versions known in the current implementation of this class
	enum RINEXversion {
		V210 = 0,		///< RINEX version V2.10
		V302,			///< RINEX version V3.02
		VALL,			///< for features applicable to all versions 
		VTBD			///< To Be Defined version 
	};
	/// RINEX label identifiers defined for each RINEX file header record
	enum RINEXlabel {
		VERSION = 0,///< "RINEX VERSION / TYPE"	(System can be: in V210=G,R,S,T,M; in V302=G,R,E,S,M) 
		RUNBY,		///< "PGM / RUN BY / DATE" (All versions)
		COMM,		///< "COMMENT" (All versions)
		MRKNAME,	///< "MARKER NAME" (All versions)
		MRKNUMBER,	///< "MARKER NUMBER" (All versions)
		MRKTYPE,	///< "MARKER TYPE" (All versions)
		AGENCY,		///< "OBSERVER / AGENCY" (All versions)
		RECEIVER,	///< "REC # / TYPE / VERS" (All versions)
		ANTTYPE,	///< "ANT # / TYPE" (All versions)
		APPXYZ,		///< "APPROX POSITION XYZ" (All versions)
		ANTHEN,		///< "ANTENNA: DELTA H/E/N" (All versions)
		ANTXYZ,		///< "ANTENNA: DELTA X/Y/Z" (in version V302)
		ANTPHC,		///< "ANTENNA: PHASECENTER" (in version V302)
		ANTBS,		///< "ANTENNA: B.SIGHT XYZ" (in version V302)
		ANTZDAZI,	///< "ANTENNA: ZERODIR AZI" (in version V302)
		ANTZDXYZ,	///< "ANTENNA: ZERODIR XYZ" (in version V302)
		COFM,		///< "CENTER OF MASS XYZ" (in version V302)
		WVLEN,		///< "WAVELENGTH FACT L1/2" (in version V210)
		TOBS,		///< "# / TYPES OF OBSERV" (in version V210)
		SYS,		///< "SYS / # / OBS TYPES" (in version V302)
		SIGU,		///< "SIGNAL STRENGTH UNIT" (in version V302)
		INT,		///< "INTERVAL" (All versions)
		TOFO,		///< "TIME OF FIRST OBS" (All versions)
		TOLO,		///< "TIME OF LAST OBS" (All versions)
		CLKOFFS,	///< "RCV CLOCK OFFS APPL" (All versions)
		DCBS,		///< "SYS / DCBS APPLIED" (in version V302)
		PCVS,		///< "SYS / PCVS APPLIED" (in version V302)
		SCALE,		///< "SYS / SCALE FACTOR" (in version V302)
		PHSH,		///< "SYS / PHASE SHIFTS" (in version V302)
		GLSLT,		///< "GLONASS SLOT / FRQ #" (in version V302)
        GLPHS,      ///< "GLONASS COD/PHS/BIS" (in version V302)
		LEAP,		///< "LEAP SECONDS" (All versions)
		SATS,		///< "# OF SATELLITES" (All versions)
		PRNOBS,		///< "PRN / # OF OBS" (All versions)

		IONA,		///< "ION ALPHA"			(in GPS NAV version V210)
		IONB,		///< "ION BETA"				(in GPS NAV version V210)
		DUTC,		///< "DELTA-UTC: A0,A1,T,W"	(in GPS NAV version V210)
		IONC,		///< "IONOSPHERIC CORR"		(in GNSS NAV version V302)
		TIMC,		///< "TIME SYSTEM CORR"		(in GNSS NAV version V302)

		EOH,		///< "END OF HEADER"
					///< PSEUDOLABELS:
		INFILEVER,	///< To access VERSION data read from an input file
		NOLABEL,	///< No lable detected (to manage error messages)
		DONTMATCH,	///< Label do not match with RINEX version (to manage error messages)
		LASTONE		///< Las item: last RINEXlabel. Also EOF found when reading.
		};
	//constructors & destructor
	RinexData(RINEXversion ver, Logger* plogger);
	RinexData(RINEXversion ver);
	RinexData(RINEXversion ver, string prg, string rby, Logger* plogger);
	RinexData(RINEXversion ver, string prg, string rby);
	~RinexData(void);
	//methods to set RINEX line header record data values storing them in the RinexData object
	bool setHdLnData(RINEXlabel rl);
	bool setHdLnData(RINEXlabel rl, RINEXlabel a, const string &b);
	bool setHdLnData(RINEXlabel rl, char a, int b, const vector<int> &c);
	bool setHdLnData(RINEXlabel rl, char a, int b, const vector<string> &c);
	bool setHdLnData(RINEXlabel rl, char a, const string &b, double c, double d, double e);
    bool setHdLnData(RINEXlabel rl, char a, const string &b, double c, const vector<string> &d);
	bool setHdLnData(RINEXlabel rl, char a, const string &b, const string &c);
//	bool setHdLnData(RINEXlabel rl, char a, const vector<string> &b);
    bool setHdLnData(RINEXlabel rl, char a, vector<string> &b);
	bool setHdLnData(RINEXlabel rl, double a, double b=0.0, double c=0.0);
	bool setHdLnData(RINEXlabel rl, int a, int b=0);
	bool setHdLnData(RINEXlabel rl, int a, int b, const vector<string> &c);
	bool setHdLnData(RINEXlabel rl, const string &a, const string &b=string(), const string &c=string());
	bool setHdLnData(RINEXlabel rl, const string &a, const vector<double> &b);
	bool setHdLnData(RINEXlabel rl, const string &a, double b, double c=0.0, int d=0, int e=0, const string &f=string(), int g=0);
	//methods to obtain line header records data stored in the RinexData object
	bool getHdLnData(RINEXlabel rl, RINEXlabel &a, string &b, unsigned int index = 0);
	bool getHdLnData(RINEXlabel rl, char &a, int &b, vector <int> &c, unsigned int index = 0);
	bool getHdLnData(RINEXlabel rl, char &a, int &b, vector <string> &c, unsigned int index = 0);
	bool getHdLnData(RINEXlabel rl, char &a, string &b, double &c, double &d, double &e);
	bool getHdLnData(RINEXlabel rl, char &a, string &b, double &c, vector <string> &d, unsigned int index = 0);
	bool getHdLnData(RINEXlabel rl, char &a, string &b, string &c, unsigned int index = 0);
    bool getHdLnData(RINEXlabel rl, char &a,  vector <string> &b, unsigned int index = 0);
	bool getHdLnData(RINEXlabel rl, double &a);
	bool getHdLnData(RINEXlabel rl, double &a, char &b, char &c);
	bool getHdLnData(RINEXlabel rl, double &a, double &b, double &c);
	bool getHdLnData(RINEXlabel rl, int &a);
    bool getHdLnData(RINEXlabel rl, int &a, double &b, string &c);
	bool getHdLnData(RINEXlabel rl, int &a, int &b, unsigned int index = 0);
	bool getHdLnData(RINEXlabel rl, string &a);
	bool getHdLnData(RINEXlabel rl, string &a, string &b);
	bool getHdLnData(RINEXlabel rl, string &a, string &b, string &c);
	bool getHdLnData(RINEXlabel rl, string &a, vector <double> &b, unsigned int index = 0);
    bool getHdLnData(RINEXlabel rl, string &a, double &b, unsigned int index = 0);
	bool getHdLnData(RINEXlabel rl, string &a, double &b, double &c, int &d, int &e, string &f, int &g, unsigned int index = 0);
	//methods to process header records data
	RINEXlabel lblTOid(string label);
	string idTOlbl(RINEXlabel id);
	RINEXlabel get1stLabelId();
	RINEXlabel getNextLabelId();
	void clearHeaderData();
	//methods to process and collect epoch data
	double setEpochTime(int weeks, double secs, double bias=0.0, int eFlag=0);
	bool saveObsData(char sys, int sat, string obsType, double value, int lol, int strg, double tTag);
	double getEpochTime(int &weeks, double &secs, double &bias, int &eFlag);
	bool getObsData(char &sys, int &sat, string &obsType, double &value, int &lol, int &strg, unsigned int index = 0);
	bool setFilter(vector<string> selSat, vector<string> selObs);
	bool filterObsData(bool removeNotPrt = false);
	void clearObsData();
	bool saveNavData(char sys, int sat, double bo[BO_MAXLINS][BO_MAXCOLS], double tTag);
	bool getNavData(char& sys, int &sat, double (&bo)[BO_MAXLINS][BO_MAXCOLS], double &tTag, unsigned int index = 0);
	bool filterNavData();
	void clearNavData();
	//methods to print RINEX files
	string getObsFileName(string prefix, string country = "---"); 
	string getNavFileName(string prefix, char suffix = 'N', string country = "---");
	void printObsHeader(FILE* out);
	void printObsEpoch(FILE* out);
	void printObsEOF(FILE* out);
	void printNavHeader(FILE* out);
	void printNavEpochs(FILE* out);
	bool hasNavEpochs(char sys);
	//methods to collect data from existing RINEX files
	RINEXlabel readRinexHeader(FILE* input);
	int readObsEpoch(FILE* input);
	int readNavEpoch(FILE* input);

private:
	struct LABELdata {	//A template for data related to each defined RINEX label and related record
		RINEXlabel labelID;	//The header label identification
		const char* labelVal;		//The RINEX label value in columns 61-80
		RINEXversion ver;	//The RINEX version where this label is defined
		unsigned int type;	//The type of the record with this label
		bool hasData;		//If there are data stored for this record or not
		string comment;
		//Constructor for most labelID (except COMM)
		LABELdata (RINEXlabel lId, const char* lVal, RINEXversion v, unsigned int t) {
			labelID = lId;
			labelVal = lVal;
			ver = v;
			type = t;
			hasData = false;
		}
		//Constructor for labelID COMM
		LABELdata (string c) {
			labelID = COMM;
			labelVal = "COMMENT";
			ver = VALL;
			type = OBSOPT + NAVOPT;
			hasData = true;
			comment = c;
		}
	};
	vector <LABELdata> labelDef;	//A place to store data for all RINEX header labels
	unsigned int labelIdIdx;		//an index to iterater over labelDef with get1stLabelId and getNextLabelId 
	//RINEX header data grouped by line type/label
	//"RINEX VERSION / TYPE"
	RINEXversion inFileVer;	//The RINEX version of the input file (when applicable)
	RINEXversion version;	//The RINEX version of the output file
	char fileType;			//V210:O, N(GPS nav), G(GLONASS nav), H(Geo nav), ...; V302:O, N, M
	string fileTypeSfx;		//a suffix to better describe the file type
	char sysToPrintId;			//System to print identifier: V210=G(GPS), R(GLO), S(SBAS), T, M(multiple); V302=G, R, E (Galileo), J, C, S, M
	string systemIdSfx;		//a suffix to better describe the system
	//"PGM / RUN BY / DATE"
	string pgm;				//Program used to create current file
	string runby;			//Who executed the program
	string date;			//Date and time of file creation
	//"MARKER NAME"
	string markerName;		//Name of antenna marker
	//"* MARKER NUMBER"
	string markerNumber;	//Number of antenna marker (HUMAN)
	//"MARKER TYPE"
	string markerType;		//Marker type as per V302
	//"OBSERVER / AGENCY"
	string observer;		//Name of observer
	string agency;			//Name of agency
	//"REC # / TYPE / VERS
	string rxNumber;		//Receiver number
	string rxType;			//Receiver type
	string rxVersion;		//Receiver version (e.g. Internal Software Version)
	//"ANT # / TYPE"
	string antNumber;		//Antenna number
	string antType;			//Antenna type
	//"APPROX POSITION XYZ"
	double aproxX;			//Geocentric approximate marker position
	double aproxY;
	double aproxZ;
	//"ANTENNA: DELTA H/E/N"
	double antHigh;		//Antenna height: Height of the antenna reference point (ARP) above the marker
	double eccEast;		//Horizontal eccentricity of ARP relative to the marker (east/north)
	double eccNorth;
	//"* ANTENNA: DELTA X/Y/Z"	V302
	double antX;
	double antY;
	double antZ;
	//"* ANTENNA: PHASECENTER"	V302
	char antPhSys;
	string antPhCode;
	double antPhNoX;
	double antPhEoY;
	double antPhUoZ;
	//"* ANTENNA: B.SIGHT XYZ"	V302
	double antBoreX;
	double antBoreY;
	double antBoreZ;
	//"* ANTENNA: ZERODIR AZI"	V302
	double antZdAzi;
	//"* ANTENNA: ZERODIR XYZ"	V302
	double antZdX;
	double antZdY;
	double antZdZ;
	//"* CENTER OF MASS XYZ"	V302
	double centerX;
	double centerY;
	double centerZ;
	//"WAVELENGTH FACT L1/2"	V210
	struct WVLNfactor {
		int wvlenFactorL1;
		int wvlenFactorL2;
		int nSats;		//number of satellites these factors apply
		vector <string> satNums;
		//constructors
		WVLNfactor () {
			wvlenFactorL1 = 1;
			wvlenFactorL2 = 1;
			nSats = 0;
		}
		WVLNfactor (int wvl1, int wvl2) {	//for the default wavelength record
			wvlenFactorL1 = wvl1;
			wvlenFactorL2 = wvl2;
			nSats = 0;
		}
		WVLNfactor (int wvl1, int wvl2, const vector<string> &sats) {
			wvlenFactorL1 = wvl1;
			wvlenFactorL2 = wvl2;
			nSats = sats.size();
			satNums = sats;
		}
	};
	vector <WVLNfactor> wvlenFactor;
	//"# / TYPES OF OBSERV"		V210
	//"SYS / # / OBS TYPES"		V302
    struct OBSmeta {    //Defines metadata to manage observables
        string id;  //identifier of each obsType type: C1C, L1C, D1C, S1C... (see RINEX V302 document: 5.1 Observation codes)
        bool sel;   //if true, the obsType is selected, that is, their data will be taken into account
        bool prt;   //if true, the obsType will be printed (it is selected and its printable in the current version)
        //constructor
        OBSmeta (string identifier, bool selected, bool printable) {
            id = identifier;
            sel = selected;
            prt = printable;
        }
    };
	struct GNSSsystem {	//Defines data for each GNSS system that can provide data to the RINEX file. Used for all versions
		char system;	//system identification: G (GPS), R (GLONASS), S (SBAS), E (Galileo). See RINEX V302 document: 3.5 Satellite numbers
		bool selSystem;	//a flag stating if the system is selected (will pass filtering or not)
        vector <OBSmeta> obsTypes;
        vector <int> selSat;       //the list of selected satelites to be printed. If empty all of them will be printed
		//constructor
		//GNSSsystem (char sys, const vector<string> &obsT) {
        GNSSsystem (char sys, vector<string> obsT) {
			system = sys;
			selSystem = true;
			//insert all obsType having equivalence in RINEX V2, initially set to NOT SELECTED and NOT PRINTABLE
            for (int i = 0;  !v3obsTypes[i].empty() ; i++) obsTypes.push_back(OBSmeta(v3obsTypes[i], false, false));
            //for each obsTypes passed in argument, it already inserted set it as SELECTED
            //if not, inset it as SELECTED and NOT PRINTABLE
            vector <OBSmeta> ::iterator itobs;
            for (vector<string>::iterator iti = obsT.begin(); iti != obsT.end(); iti++) {
                for (itobs = obsTypes.begin(); (itobs != obsTypes.end()) && ((*iti).compare(itobs->id) != 0); itobs++);
                if (itobs != obsTypes.end()) itobs->sel = true;
                else obsTypes.push_back(OBSmeta(*iti, true, false));
            }
        };
	};
	vector <GNSSsystem> systems;
	//"* SIGNAL STRENGTH UNIT"	V302
	string signalUnit;
	//"* INTERVAL"				VALL
	double obsInterval;
	//"TIME OF FIRST OBS"		VALL
	int firstObsWeek;
	double firstObsTOW;
	string obsTimeSys;
	//"* TIME OF LAST OBS"		VALL
	int lastObsWeek;
	double lastObsTOW;
	//"* RCV CLOCK OFFS APPL"		VALL
	int rcvClkOffs;
	//"* SYS / DCBS APPLIED"		V302
	struct DCBSPCVSapp {	//defines data for corrections of differential code biases (DCBS)
							//or corrections of phase center variations (PCVS)
		int sysIndex;		//the system index in vector systems
		string corrProg;	//Program name used to apply corrections
		string corrSource;	//Source of corrections
		//constructors
		DCBSPCVSapp () {
		};
		DCBSPCVSapp (int oi, string cp, const string &cs) {
			sysIndex = oi;
			corrProg = cp;
			corrSource = cs;
		};
	};
	vector <DCBSPCVSapp> dcbsApp;
	//"*SYS / PCVS APPLIED		V302
	vector <DCBSPCVSapp> pcvsApp;
	//"* SYS / SCALE FACTOR"	V302
	struct OSCALEfact {	//defines scale factor applied to observables
		int sysIndex;	//the system index in vector systems
		int factor;		//a factor to divide stored observables with before use (1,10,100,1000)
		vector <string> obsType;	//the list of observable types involved. If vector is empty, all observable types are involved
        //constructor
		OSCALEfact (int oi, int f, const vector<string> &ot) {
			sysIndex = oi;
			factor = f;
			obsType = ot;
		};
	};
	vector <OSCALEfact> obsScaleFact;
	//"* SYS / PHASE SHIFTS		V302
	struct PHSHcorr {	//defines Phase shift correction used to generate phases consistent w/r to cycle shifts
		int sysIndex;	//the system index in vector systems
		string obsCode;	//Carrier phase observable code (Type-Band-Attribute)
		double correction;	//Correction applied (cycles)
		vector <string> obsSats;	//the list of satellites involved. If vector is empty, all system satellites are involved
		//constructor
		PHSHcorr (int oi, string cd, double co, const vector<string> &os) {
			sysIndex = oi;
			obsCode = cd;
			correction = co;
			obsSats = os;
		};
	};
	vector <PHSHcorr> phshCorrection;
	//* GLONASS SLOT / FRQ #
	struct GLSLTfrq {	//defines Glonass slot and frequency numbers
		int slot;		//slot
		int frqNum;		//Frequency numbers (-7...+6)
		//constructor
		GLSLTfrq (int sl, int fr) {
			slot = sl;
			frqNum = fr;
		};
	};
	vector <GLSLTfrq> gloSltFrq;
	//* GLONASS COD/PHS/BIS
    struct GLPHSbias {
        string obsCode;   //the observation code
        double obsCodePhaseBias;    //the code phase bias correction
        //constructor
        GLPHSbias (string oc, double phb) {
            obsCode = oc;
            obsCodePhaseBias = phb;
        }
    };
    vector <GLPHSbias> gloPhsBias;
	//"* LEAP SECONDS"			VALL
	int leapSec;
	int deltaLSF;		//V302 only
	int weekLSF;		//V302 only
	int dayLSF;			//V302 only
	//"* # OF SATELLITES"		VALL
	int numOfSat;
	//"* PRN / # OF OBS"			VALL
	struct PRNobsnum {	//defines prn and number of observables for each observable type
		char sysPrn;	//the system the satellite prn belongs
		int	satPrn;		//the prn number of the satellite
		vector <int> obsNum;	//the number of observables for each observable type
		//constructors
		PRNobsnum () {
		};
		PRNobsnum (char s, int p, const vector<int> &o) {
			sysPrn = s;
			satPrn = p;
			obsNum = o;
		};
	};
	vector <PRNobsnum> prnObsNum;
	//"* IONOSPHERIC CORR		(in GNSS NAV version V302)
	struct IONOcorr {	//defines ionospheric correction parameters
		string corrType;	//Correction type GAL(Galileo:ai0-ai2),GPSA(GPS:alpha0-alpha3),GPSB(GPS:beta0-beta3)
		vector <double> corrValues;
		//constructors
		IONOcorr () {
		};
		IONOcorr (string ct, const vector<double> &corr) {
			corrType = ct;
			corrValues = corr;
		};
	};
	vector <IONOcorr> ionoCorrection;
	//"* TIME SYSTEM CORR		(in GNSS NAV version V302)
	struct TIMcorr {	//defines correctionsto transform the system time to UTC or other time systems
		string corrType;	//Correction type: GAUT, GPUT, SBUT, GLUT, GPGA, GLGP
		double a0, a1;		//Coefficients of 1-deg polynomial
		int refTime;		//reference time for polynomial (seconds into GPS/GAL week)
		int refWeek;		//Reference week number
		string sbas;		//EGNOS, WAAS, or MSAS, or Snn (with nn = PRN-100 of satellite
		int utcId;			//UTC identifier
		//constructors
		TIMcorr () {
		};
		TIMcorr (string ct, double ca0, double ca1, int rs, int rw, const string &sb, int ui) {
			corrType = ct;
			a0 = ca0;
			a1 = ca1;
			refTime = rs;
			refWeek = rw;
			sbas = sb;
			utcId = ui;
		};
	};
	vector <TIMcorr> timCorrection;
	int deltaUTCt;
	int deltaUTCw;
	//Epoch time parameters
	int epochWeek;			//Extended (0 to NO LIMIT) GPS/GAL week number of current epoch
	double epochTOW;		//Seconds into the current week, accounting for clock bias, when the current measurement was made
	double epochClkOffset;	//Recieve clock offset applied to epoch time and measurements
	//Epoch observable data
	int epochFlag;		//The type of data following this epoch record (observation, event, ...). See RINEX definition
	int nSatsEpoch;		//Number of satellites or special records in current epoch
	double epochTimeTag;	//A tag to identify the measurements of a given epoch. Could be the estimated time of current epoch before fix
	struct SatObsData {	//defines data storage for a satellite observable (pseudorrange, phase, ...) in an epoch.
		unsigned int sysIndex;		//the system this observable belongs: its index in systems vector (see above)
		int satellite;		//the satellite this observable belongs: PRN of satellite
		unsigned int obsTypeIndex;	//the observable type: its index in obsType vector (inside the GNSSsystem object referred by sysIndex)
 		double obsValue;	//the value of this observable
		int lossOfLock;		//if loss of lock happened when observable was taken
		int strength;		//the signal strength when observable was taken
		//constructor
		SatObsData (double obsTag, unsigned int sysIdx, int sat, unsigned int obsIdx, double obsVal, int lol, int str) {
			sysIndex = sysIdx;
			satellite = sat;
			obsTypeIndex = obsIdx;
			obsValue = obsVal;
			lossOfLock = lol;
			strength = str;
		};
		//define operator for comparisons and sorting
		bool operator < (const SatObsData& param) const {
			if (sysIndex < param.sysIndex) return true;
			if (sysIndex > param.sysIndex) return false;
			//same system
			if (satellite < param.satellite) return true;
			if (satellite > param.satellite) return false;
			//same system and satellite
			if (obsTypeIndex <= param.obsTypeIndex) return true;
			return false;
		};
	};
	vector <SatObsData> epochObs;	//A place to store observable data (pseudorange, phase, ...) for one epoch
	//Epoch navigation data
	struct SatNavData {	//defines storage for navigation data for a given GNSS satellite
		double navTimeTag;	//a tag to identify the epoch of this data
		char systemId;	//the system identification (G, E, R, ...)
		int satellite;	//the PRN of the satellite navigation data belong
		double broadcastOrbit[BO_MAXLINS][BO_MAXCOLS];	//the eigth lines of RINEX navigation data, with four parameters each
		//constructor
		SatNavData(double tT, char sys, int sat, double bo[BO_MAXLINS][BO_MAXCOLS]) {
			navTimeTag = tT;
			systemId = sys;
			satellite = sat;
			for (int i=0; i<BO_MAXLINS; i++)
				for (int j=0; j<BO_MAXCOLS; j++)
					broadcastOrbit[i][j] = bo[i][j];
		};
		//define operator for comparisons and sorting
		bool operator < (const SatNavData &param) const {
			if(navTimeTag < param.navTimeTag) return true;
			if(navTimeTag > param.navTimeTag) return false;
			//same time tag
			if (systemId < param.systemId) return true;
			if (systemId > param.systemId) return false;
			//same time tag and system
			if (satellite < param.satellite) return true;
			return false;
		};
	};
	vector <SatNavData> epochNav;		//A place to store navigation data for one epoch
	//A state variable used to store reference to the label of the last record which data has been modified
	vector<LABELdata>::iterator lastRecordSet;
	unsigned int numberV2ObsTypes;
	//Logger
	Logger* plog;		//the place to send logging messages
	bool dynamicLog;	//true when created dynamically here, false when provided externally

	//private methods
	void setDefValues(RINEXversion v, Logger* p);
	string fmtRINEXv2name(string designator, int week, double tow, char ftype);
	string fmtRINEXv3name(string designator, int week, double tow, char ftype, string country);
	void setLabelFlag(RINEXlabel label, bool flagVal=true);
	bool getLabelFlag(RINEXlabel);
	RINEXlabel checkLabel(char *);
	string valueLabel(RINEXlabel label, string toAppend = string());
	int readV2ObsEpoch(FILE* input);
	int readV3ObsEpoch(FILE* input);
	int readObsEpochEvent(FILE* input, bool wrongDate);
	void printHdLineData (FILE* out, vector<LABELdata>::iterator lbIter);
	bool printSatObsValues(FILE* out, RINEXversion ver);
	RINEXlabel readHdLineData(FILE* input);
	bool readRinexRecord(char* rinexRec, int recSize, FILE* input);
	bool isSatSelected(int sysIx, int sat);
    unsigned int getSysIndex(char sysId);
	int systemIndex(char sysCode);
	string getSysDes(char s);
	void setSuffixes();
	void setSysToPrintId(string msg);
};
#endif

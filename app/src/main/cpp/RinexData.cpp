/** @file RinexData.cpp
 * Contains the implementation of the RinexData class.
 */
#include "RinexData.h"

#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <cstdio>
//from CommonClasses
#include "Utilities.h"

/**RinexData constructor providing only the minimum data required: the RINEX file version to be generated.
 *
 * Version parameter is needed in the header record RINEX VERSION / TYPE: which is mandatory in any RINEX file. Note that version
 * affects the header records that must / can be printed in the RINEX header, and the format of epoch observation data.
 * <p>A VTBD in version means that it will be defined in a further step of the process.
 * <p>Logging data are sent to the stderr.
 *
 * @param ver the RINEX version to be generated
 */
RinexData::RinexData(RINEXversion ver) {
	plog = new Logger();
	dynamicLog = true;
	setDefValues(ver, plog);
}

/**RinexData constructor providing the RINEX file version to be generated and the Logger for logging messages.
 *
 * Version parameter is needed in the header record RINEX VERSION / TYPE, which is mandatory in any RINEX file. Note that version
 * affects the header records that must / can be printed in the RINEX header, and the format of epoch observation data.
 * <p>A VTBD in version means that it will be defined in a further step of the process.
 *
 * @param ver the RINEX version to be generated
 * @param plogger a pointer to a Logger to be used to record logging messages
 */
RinexData::RinexData(RINEXversion ver, Logger* plogger) {
	dynamicLog = false;
	setDefValues(ver, plogger);
}

/**RinexData constructor providing data on RINEX version to be generated, the program being used, and who is running the program.
 *
 * Version parameter is needed in the header record RINEX VERSION / TYPE, which is mandatory in any RINEX file. Note that version
 * affects the header records that must / can be printed in the RINEX header, and the format of epoch observation data.
 * <p>A VTBD in version means that it will be defined in a further step of the process.
 * <p>Also data is passed for the recod PGM / RUN BY / DATE.
 * <p>Logging data are sent to the stderr.
 *
 * @param ver the RINEX version to be generated. (V210, V304, TBD)
 * @param prg the program used to create the RINEX file
 * @param rby who executed the program (run by)
 */
RinexData::RinexData(RINEXversion ver, string prg, string rby) {
	plog = new Logger();
	dynamicLog = true;
	setDefValues(ver, plog);
	//assign values to class data members from arguments passed 
	pgm = prg;
	runby = rby;
	setLabelFlag(RUNBY);
}

/**RinexData constructor providing data on RINEX version to be generated, the program being used, who is running the program, and the Logger for logging messages.
 *
 * Version parameter is needed in the header record RINEX VERSION / TYPE, which is mandatory in any RINEX file. Note that version
 * affects the header records that must / can be printed in the RINEX header, and the format of epoch observation data.
 * <p>A VTBD in version means that it will be defined in a further step of the process.
 * <p>Also data is passed for the recod PGM / RUN BY / DATE.
 *
 * @param ver the RINEX version to be generated. (V210, V304, TBD)
 * @param prg the program used to create the RINEX file
 * @param rby who executed the program (run by)
 * @param plogger a pointer to a Logger to be used to record logging messages
 */
RinexData::RinexData(RINEXversion ver, string prg, string rby, Logger* plogger) {
	dynamicLog = false;
	setDefValues(ver, plogger);
	//assign values to class data members from arguments passed 
	pgm = prg;
	runby = rby;
	setLabelFlag(RUNBY);
}

/**Destructor.
 */
RinexData::~RinexData(void) {
	if (dynamicLog) delete plog;
}

//PUBLIC METHODS

///a macro to assign in setHdLnData the value of the method parameter a to the given member
#define SET_1PARAM(LABEL_rl, FROM_PARAM_a) \
	FROM_PARAM_a = a; \
	setLabelFlag(LABEL_rl); \
	return true;
///a macro to assign in setHdLnData the value of the method parameters a and b to the given members
#define SET_2PARAM(LABEL_rl, FROM_PARAM_a, FROM_PARAM_b) \
	FROM_PARAM_a = a; \
	FROM_PARAM_b = b; \
	setLabelFlag(LABEL_rl); \
	return true;
///a macro to assign in setHdLnData the value of the method parameters a, b and c to the given members
#define SET_3PARAM(LABEL_rl, FROM_PARAM_a, FROM_PARAM_b, FROM_PARAM_c) \
	FROM_PARAM_a = a; \
	FROM_PARAM_b = b; \
	FROM_PARAM_c = c; \
	setLabelFlag(LABEL_rl); \
	return true;
///a macro to assign in setHdLnData the value of the method not-null string parameters a and b to the given members
#define SET_2STRPARAM(LABEL_rl, FROM_PARAM_a, FROM_PARAM_b) \
	if (a.length() != 0) FROM_PARAM_a = a; \
	if (b.length() != 0) FROM_PARAM_b = b; \
	setLabelFlag(LABEL_rl); \
	return true;
///a macro to assign in setHdLnData the value of the method not-null string parameters a, b and c to the given members
#define SET_3STRPARAM(LABEL_rl, FROM_PARAM_a, FROM_PARAM_b, FROM_PARAM_c) \
	if (a.length() != 0) FROM_PARAM_a = a; \
	if (b.length() != 0) FROM_PARAM_b = b; \
	if (c.length() != 0) FROM_PARAM_c = c; \
	setLabelFlag(LABEL_rl); \
	return true;

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier values in this overload can be:
 *  - COMM: to set a comment record content to be inserted in the RINEX header just before a given record.
 * The comment will be inserted before the first match of the the given record identifier in param a.
 * If no match occurs, comment is inserted just before the "END OF HEADER" record.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a is the label identifier stating the position for comment insertion
 * @param b the comment to be inserted
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, RINEXlabel a, const string &b) {
	switch(rl) {
	case COMM:
		for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it)
			if((it->labelID == a) || (it->labelID == EOH)) {
				lastRecordSet = labelDef.insert(it, LABELdata(b));
				return true;
			}
		return false;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier value can be:
 * - IONA: to set in RINEX GPS navigation header ionosphere parameters for the ION ALPHA (V2) record.
 * 		For this record, the meaning of the parameters will be:
 * 		- a: ignored. It is set to IONC_GPSA
 * 		- b: Ionosphere parameters [0-3] A0-A3
 * 		- c: TOW for time mark
 * 		- d: satellite identifier
 * - IONB: to set in RINEX GPS navigation header ionosphere parameters for the ION BETA (V2) record.
 * 		For this record, the meaning of the parameters will be:
 * 		- a: ignored. It is set to IONC_GPSB
 * 		- b: Ionosphere parameters [0-3] B0-B3
 * 		- c: TOW for time mark
 * 		- d: satellite identifier
 * - IONC: to set in RINEX GNSS navigation header ionosphere parameters for the IONOSPHERIC CORR (V3) record.
 * 		For this record, the meaning of the parameters will be:
 * 		- a: the label identifier of the correction type (IONC_GAL, IONC_GPSA, IONC_GPSB, ... IONC_IRNB)
 * 		- b: Ionosphere parameters [0-3] as per the correction type
 * 		- c: TOW for time mark
 * 		- d: satellite identifier
 * - DUTC: to set in RINEX GPS navigation header time correction parameters for the DELTA-UTC: A0,A1,T,W (V2 record)
 * 		For this record, the meaning of the parameters will be:
 * 		- a: Ignored. It is equivalent to set it to TIMC_GPUT
 * 		- b: Delta UTC parameters [0-1] A0,A1, [2] TOW for time mark, [3] week number for the time mark
 * 		- c: id UTC source
 * 		- d: id satellite number
 * - CORRT: to set in RINEX GLONASS navigation header time correction parameters for the CORR TO SYSTEM TIME (V2 record)
 * 		For this record, the meaning of the parameters will be:
 * 		- a: Ignored. It is equivalent to set it to TIMC_GLUT
 * 		- b: correction values [0] -TauC, [1] 0, [2] TOW for time mark, [3] week number for the time mark
 * 		- c: id UTC source
 * 		- d: id satellite number
 * - GDUTC: to set in RINEX GEO navigation header time correction parameters for the D-UTC A0,A1,T,W,S,U (V2 record)
 * 		For this record, the meaning of the parameters will be:
 * 		- a: Ignored. It is equivalent to set it to TIMC_SBUT
 * 		- b: D-UTC parameters [0-1] A0,A1, [2] TOW for time mark, [3] week number for the time mark
 * 		- c: id UTC source
 * 		- d: id satellite number
 * - TIMC: to set in RINEX GNSS navigation header time correction parameters for the TIME SYSTEM CORR (V3 record.
 * 		For this record, the meaning of the parameters will be:
 * 		- a: the label identifier of the correction type (TIMC_GPUT, TIMC_GLUT, ... TIMC_IRGP)
 * 		- b: Corrections: [0-1] coefficients a0-a1 as per the correction type, [2] TOW for time mark, [3] week number for the time mark
 * 		- c: id UTC source
 * 		- d: id satellite number
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a meaning depends on the label identifier
 * @param b meaning depends on the label identifier
 * @param c meaning depends on the label identifier
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, RINEXlabel a, const double (&b)[4], int c, int d) {
    //ignore corrections with values zero
    bool empty = true;
    for (int i = 0; empty && i < 4; ++i) empty = b[i] == 0.0;
    if (empty) return false;
    //convert V2 labels to equivalent V3 ones
    switch (rl) {
        case IONA:   //GPS iono alpha
		    rl = IONC;
		    a = IONC_GPSA;
            setLabelFlag(IONA);
            break;
        case IONB:    //GPS iono beta
		    rl = IONC;
		    a = IONC_GPSB;
		    setLabelFlag(IONB);
		    break;
        case DUTC:    //GPS UTC correction
		    rl = TIMC;
		    a = TIMC_GPUT;
            setLabelFlag(DUTC);
            break;
        case CORRT:    //GLONASS UTC correction
            rl = TIMC;
            a = TIMC_GLUT;
            setLabelFlag(CORRT);
            break;
        case GEOT:    //GEO UTC correction
            rl = TIMC;
            a = TIMC_SBUT;
            setLabelFlag(GEOT);
            break;
        default:
            break;
    }
    switch(rl) {
		case IONC:
        case TIMC:
			//check if these data have been already stored
			for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); it++) {
				if ((it->corrType == a)
                    && (it->corrValues[0] == b[0])
                    && (it->corrValues[1] == b[1])
                    && (it->corrValues[2] == b[2])
                    && (it->corrValues[3] == b[3])) return false;
			}
			corrections.push_back(CORRECTION(a, b, c, d));
			setLabelFlag(rl);
            return true;
		default:
			throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier value in this overload can be:
 *  - PRNOBS: to set in RINEX header data for record PRN / # OF OBS. Note that number of observables for each
 *            observation type (param c vector) shall be in the same order as per observable types for this system in the
 * "# / TYPES OF OBSERV" or "SYS / # / OBS TYPES".
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a the system the prn satellite belongs
 * @param b the prn number of the satellite
 * @param c a vector with the number of observables for each observable type
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, char a, int b, const vector<int> &c) {
	switch(rl) {
	case PRNOBS:
		prnObsNum.push_back(PRNobsnum(a, b, c));
		setLabelFlag(PRNOBS);
		return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}
/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier values in this overload can be:
 *  - TOFO: to set the current epoch time (week and TOW) as the fist observation time. Data to be included in record TIME OF FIRST OBS
 *  - TOLO: to set the current epoch time (week and TOW) as the last observation time. Data to be included in record TIME OF LAST OBS
 *
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a is the system identifier (ignored if rl is TOLO)
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, char a) {
	switch(rl) {
	case TOFO:
        firstObsWeek = epochWeek;
        firstObsTOW = epochTOW;
        obsTimeSys = a;
        setLabelFlag(TOFO);
        return true;
    case TOLO:
        lastObsWeek = epochWeek;
        lastObsTOW = epochTOW;
        setLabelFlag(TOLO);
        return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier value in this overload can be:
 *  - SCALE: to set in RINEX header data for the scale factors used in the observable data. Data to be included in record SYS / SCALE FACTOR.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a the system identifier
 * @param b a factor to divide stored observables before use (1,10,100,1000)
 * @param c the list of observable types involved. If vector is empty, all observable types are involved
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, char a, int b, const vector<string> &c) {
	int n;
	switch(rl) {
	case SCALE:
		if ((n = systemIndex(a)) < 0) return false;
		obsScaleFact.push_back(OSCALEfact(n, b, c));
		setLabelFlag(SCALE);
		return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier value in this overload can be:
 *  - ANTPHC: to set in RINEX header the average phase center position w/r to antenna reference point (m). Data to be included in record ANTENNA: PHASECENTER.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a the system identifier
 * @param b the observable code
 * @param c the North (fixed station) or X coordinate in body-fixed coordinate system (vehicle)
 * @param d the East (fixed station) or Y coordinate in body-fixed coordinate system (vehicle)
 * @param e the Up (fixed station) or Z coordinate in body-fixed coordinate system (vehicle)
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, char a, const string &b, double c, double d, double e) {
	switch(rl) {
	case ANTPHC:
		antPhEoY = d;
		antPhUoZ = e;
		SET_3PARAM(ANTPHC, antPhSys, antPhCode, antPhNoX)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier value in this overload can be:
 *  - PHPS: to set in RINEX header the phase shift correction used to generate phases consistent w/r to cycle shifts.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a the system identifier
 * @param b the observable code
 * @param c the correction applied in cycles
 * @param d the list of satellites (Snn)
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, char a, const string &b, double c, const vector<string> &d) {
    int sysInx;
    switch(rl) {
        case PHSH:
            if ((sysInx = systemIndex(a)) >= 0) {
                phshCorrection.push_back(PHSHcorr(sysInx, b, c, d));
                setLabelFlag(PHSH);
            } else return false;
            break;
        default:
            throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
    }
    return true;
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier value in this overload can be:
 *  - DCBS: to set in RINEX header data for corrections of differential code biases (DCBS). Data to be included in record SYS / DCBS APPLIED.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a the system (G/R/E/S) observable belongs
 * @param b the program name used to apply corrections
 * @param c the source of corrections
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, char a, const string &b, const string &c) {
	int n;
	switch(rl) {
	case DCBS:
		if ((n = systemIndex(a)) < 0) return false;
		dcbsApp.push_back(DCBSPCVSapp(n, b, c));
		setLabelFlag(DCBS);
		return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier values in this overload can be:
 *  - SYS: to set data for the given system as required in "SYS / # / OBS TYPES" records
 *  - TOBS: to set data for the given system as required in "# / TYPES OF OBSERV" record
 * <p> Note that arguments use notation according to RINEX V304 for system identification and observable types.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a the system identification: G (GPS), R (GLONASS), S (SBAS), E (Galileo), ...
 * @param b a vector with identifiers for each observable type (C1C, L1C, D1C, S1C...) contained in epoch data for this system
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, char a,  vector<string> &b) {
    int sysIndex;
    bool isNew;
	switch(rl) {
	case SYS:
	case TOBS:
        sysIndex = systemIndex(a);
        if (sysIndex < 0) {
        	//a new system and its observables is inserted
            systems.push_back(GNSSsystem(a, b));
            setLabelFlag(SYS);
            setLabelFlag(TOBS);
        } else {
        	//the system already exists, insert the new observables
            for (vector<string>::iterator itNewObs = b.begin(); itNewObs != b.end(); itNewObs++) {
                isNew = true;
                for (vector<OBSmeta>::iterator itObs = systems[sysIndex].obsTypes.begin(); itObs != systems[sysIndex].obsTypes.end(); itObs++) {
                    if ((*itNewObs).compare(itObs->id) == 0) {
                        isNew = false;
                        break;
                    }
                }
                if (isNew) {
					systems[sysIndex].obsTypes.push_back(OBSmeta(*itNewObs, true, false));
                }
            }
        }
        return true;
    default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier values in this overload can be:
 *  - ANTZDAZI: to set the azimuth of the zero-direction of a fixed antenna (degrees in param a, from north). Data to be included in record ANTENNA: ZERODIR AZI.
 *  - INT: to set in RINEX header the time interval of measurements (in param a), that is, the time interval in seconds between two consecutive epochs
 *  - ANTHEN: to set antenna data for record "ANTENNA: DELTA H/E/N". Param a is the height of the antenna reference point (ARP) above the marker. Param b is 
 * the antenna horizontal eccentricity of ARP relative to the marker east. Param c is the antenna horizontal eccentricity of ARP relative to the marker north.
 *  - APPXYZ: to set APROX POSITION record data for this RINEX file header. Params a, b an c are respectively the X, Y and Z oordinates of the position.
 *  - ANTXYZ: to set in RINEX header the position of antenna reference point for antenna on vehicle (m). Data to be included in record ANTENNA: DELTA X/Y/Z. 
 * Params a, b an c are respectively the X, Y and Z oordinates of the position in body-fixed coordinate system.
 *  - ANTBS: to set in RINEX header the direction of the �vertical� antenna axis towards the GNSS satellites. Data to be included in record ANTENNA: B.SIGHT XYZ.
 * Params a, b an c are respectively the X, Y and Z oordinates in body-fixed coordinate system.
 *  - ANTZDXYZ: to set in RINEX header the zero-direction of antenna antenna on vehicle. Data to be included in record ANTENNA: ZERODIR XYZ.
 * Params a, b an c are respectively the X, Y and Z oordinates in body-fixed coordinate system.
 *  - COFM: to set in RINEX header the current center of mass (X,Y,Z, meters) of vehicle in body-fixed coordinate system. Data to be included in record CENTER OF MASS: XYZ.
 * Params a, b an c are respectively the X, Y and Z oordinates in body-fixed coordinate system.
 *  - VERSION : to set the RINEX version to be generated. Param a is the version to be generated: V210 if 2.0 < a, V304 if 3.0 < a, VTBD otherwise.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a meaning depends on the label identifier 
 * @param b meaning depends on the label identifier
 * @param c meaning depends on the label identifier
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, double a, double b, double c) {
	switch(rl) {
	case ANTZDAZI:
		SET_1PARAM(ANTZDAZI, antZdAzi)
	case INT:
		SET_1PARAM(INT, obsInterval)
	case ANTHEN:
		SET_3PARAM(ANTHEN, antHigh, eccEast, eccNorth)
	case APPXYZ:
		SET_3PARAM(APPXYZ, aproxX, aproxY, aproxZ)
	case ANTXYZ:
		SET_3PARAM(ANTXYZ, antX, antY, antZ)
	case ANTBS:
		SET_3PARAM(ANTBS, antBoreX, antBoreY, antBoreZ)
	case ANTZDXYZ:
		SET_3PARAM(ANTZDXYZ, antZdX, antZdY, antZdZ)
	case COFM:
		SET_3PARAM(COFM, centerX, centerY, centerZ)
	case VERSION:
		version = VTBD;
		if (a > 2.0) version = V210;
		if (a > 3.0) version = V304;
		return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier values in this overload can be:
 * - CLKOFFS to set in RINEX header if the realtime-derived receiver clock offset is applied or not (value in param a).
 * 		Data to be included in record RCV CLOCK OFFS APPL.
 * 		Params b to e are ignored.
 * - LEAP to set in RINEX header the number of leap seconds, being:
 *		a: value of leap seconds
 *		b: Future or past leap seconds ΔtLSF
 *		c: Respective week number WN_LSF
 *		d: Respective day number DN
 *		e: Time system identifier
 * - SATS to set in RINEX header the number of satellites for which observables are stored in the file.
 * 		a: the number of satellites, to be included in record # OF SATELLITES.
 * 		Params b to e are ignored.
 * - WVLEN to set WAVELENGTH FACT L1/2 default record data for GPS Observation RINEX file header.
 * 		a: the wave length factor for L1
 *		b: the wave length factor for L2
 * 		Params c to e are ignored.
 * - GLSLT to set Glonas slot data for record "GLONASS SLOT / FRQ #" (in version V304):
 *		a: the slot number
 *		b: the corresponding frequency numbers (-7...+6)
 * 		Params c to e are ignored.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a meaning depends on the label identifier 
 * @param b meaning depends on the label identifier
 * @param c meaning depends on the label identifier
 * @param d meaning depends on the label identifier
 * @param e meaning depends on the label identifier
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, int a, int b, int c, int d, char e) {
	switch(rl) {
	case CLKOFFS:
		SET_1PARAM(CLKOFFS, rcvClkOffs)
	case LEAP:
		if (e == ' ') e = 'G';	//by default GPS
		for (vector<LEAPsecs>::iterator it = leapSecs.begin(); it != leapSecs.end(); it++) {
		    //check if values already set
			if ((it->sysId == e)
                && (it->secs == a)
                && (it->deltaLSF == b)
                && (it->weekLSF == c)
                && (it->dayLSF == d)) return false;
		}
		leapSecs.push_back(LEAPsecs(a, b, c, d, e));
		setLabelFlag(LEAP);
		return true;
	case SATS:
		SET_1PARAM(SATS, numOfSat)
	case WVLEN:
		if (wvlenFactor.empty()) wvlenFactor.push_back(WVLNfactor(a, b));   //insert default record
		else if (wvlenFactor[0].satNums.empty()) {    //replace current values of factors
			wvlenFactor[0].wvlenFactorL1 = a;
			wvlenFactor[0].wvlenFactorL2 = b;
		} else {    //insert the default values in the first position
		    wvlenFactor.insert(wvlenFactor.begin(), WVLNfactor(a, b));
		}
	 	setLabelFlag(WVLEN);
		return true;
	case GLSLT:
        gloSltFrq.push_back(GLSLTfrq(a,b));
        setLabelFlag(GLSLT);
        return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier value in this overload can be:
 * - WVLEN to set optional WAVELENGTH FACT L1/2 records data for the RINEX file header.
 *      Note that those optional records are for RINEX V2.1 files only.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a the wave length factor for L1
 * @param b the wave length factor for L2
 * @param c vector with the identification (system + PRNs) of a satellite in each element the factor will apply
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, int a, int b, const vector<string> &c) {
	switch(rl) {
	case WVLEN:
		wvlenFactor.push_back(WVLNfactor(a, b, c));
	 	setLabelFlag(WVLEN);
		return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 *
 * The label identifier values in this overload can be:
 * - RECEIVER: to set GNSS receiver data for the "REC # / TYPE / VERS" obligatory records in the RINEX file header.
 * Param a is the receiver number, param b is the receiver type, and param c the receiver version.
 * - AGENCY: to set OBSERVER / AGENCY record data of the RINEX file header.
 * Param a is the observer name, and param b the agency name. Param c is ignored.
 * - ANTTYPE: to set antenna data for "ANT # / TYPE" record in the RINEX file header.
 * Param a is the antenna number, and param b the antenna type. Param c is ignored.
 * - RUNBY: to set data on program and who executed it for record "PGM / RUN BY / DATE" in the RINEX file header.
 * Param a is program used to create current file, and param b who executed the program. Param c is ignored.
 * - SIGU: to set in RINEX header the unit of the signal strength observables Snn (if present). Data to be included in record SIGNAL STRENGTH UNIT.
 * Param a is the unit of the signal strength. Params b and c are ignored.
 * - MRKNAME: to set MARKER data for the RINEX file header. Data to be included in MARKER NAME
 * Param a is the marker name. Params b and c are ignored.
 * - MRKNUMBER: to set marker data for the RINEX file header. Data to be included in MARKER NUMBER
 * Param a is the marker number. Params b and c are ignored.
 * - MRKTYPE: to set marker data for the RINEX file header. Data to be included in MARKER TYPE records.
 * Param a is the marker type. Params b and c are ignored.
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a meaning depends on the label identifier 
 * @param b meaning depends on the label identifier
 * @param c meaning depends on the label identifier
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, const string &a, const string &b, const string &c) {
	switch(rl) {
	case RECEIVER:
		SET_3STRPARAM(RECEIVER, rxNumber, rxType, rxVersion);
	case AGENCY:
		SET_2STRPARAM(AGENCY, observer, agency)
	case ANTTYPE:
		SET_2PARAM(ANTTYPE, antNumber, antType)
	case RUNBY:
		SET_3STRPARAM(RUNBY, pgm, runby, date)
	case SIGU:
		SET_1PARAM(SIGU, signalUnit)
	case MRKNAME:
		SET_1PARAM(MRKNAME, markerName)
	case MRKNUMBER:
		SET_1PARAM(MRKNUMBER, markerNumber)
	case MRKTYPE:
		SET_1PARAM(MRKTYPE, markerType)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**setHdLnData sets data values for RINEX file header records
 * The label identifier value in this overload can be:
 * - GLPHS to set GLONASS phase bias correction used to align code and phase observations
 *		The meaning of parameters for this case hare:
 *		a the GLONASS signal identifier (C1C, C1P, C2C, C2P)
 *		b the code phase bias correction
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a meaning depends on the label identifier
 * @param b meaning depends on the label identifier
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::setHdLnData(RINEXlabel rl, const string &a, double b) {
	switch(rl) {
	case GLPHS:
		gloPhsBias.push_back(GLPHSbias(a, b));
		setLabelFlag(GLPHS);
		return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}
#undef SET_1PARAM
#undef SET_2PARAM
#undef SET_3PARAM

///a macro to assign in getHdLnData the value of the given member to the method parameter a
#define GET_1PARAM(LABEL_rl, TO_PARAM_a) \
	a = TO_PARAM_a; \
	return getLabelFlag(LABEL_rl);
///a macro to assing the values of the given members to the method parameters a and b
#define GET_2PARAM(LABEL_rl, TO_PARAM_a, TO_PARAM_b) \
	a = TO_PARAM_a; \
	b = TO_PARAM_b; \
	return getLabelFlag(LABEL_rl);
///a macro to assing the values of the given members to the method parameters a, b and c
#define GET_3PARAM(LABEL_rl, TO_PARAM_a, TO_PARAM_b, TO_PARAM_c) \
	a = TO_PARAM_a; \
	b = TO_PARAM_b; \
	c = TO_PARAM_c; \
	return getLabelFlag(LABEL_rl);
///a macro to assing the values of the given members to the method parameters a, b, c and d
#define GET_4PARAM(LABEL_rl, TO_PARAM_a, TO_PARAM_b, TO_PARAM_c, TO_PARAM_d) \
	a = TO_PARAM_a; \
	b = TO_PARAM_b; \
	c = TO_PARAM_c; \
	d = TO_PARAM_d; \
	return getLabelFlag(LABEL_rl);

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - TOFO: to get the epoch week and TOW of the fist observation time.
 * - TOLO: to get the epoch week and TOW of the last observation time.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a is the GNSS week number
 * @param b is the GNSS time of week (TOW)
 * @param c is the system identifier of the time system
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, int &a, double &b, char &c) {
	switch(rl) {
	case TOFO:
		GET_3PARAM(TOFO, firstObsWeek, firstObsTOW, obsTimeSys)
	case TOLO:
		GET_3PARAM(TOLO, lastObsWeek, lastObsTOW, obsTimeSys)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - IONC (V3) to get from RINEX nav header ionospheric corrections of the type indicated
 * - IONA, (V2) a synonim for IONC correction of type IONC_GPSA (V3)
 * - IONB, (V2) a synonim for IONC correction of type IONC_GPSB (V3) corrections
 * - TIMC (V3) to get from RINEX nav header time corrections of the type indicated
 * - DUTC, (V2) a synonim for TIMC correction of type TIMC_GPUT (V3)
 * - CORRT, (V2) a synonim for TIMC correction of type TIMC_GLUT (V3) corrections
 * - GEOT, (V2) a synonim for TIMC correction of type TIMC_SBUT (V3) corrections
 * This method extracts from the vector containing all correction the one of the type indicated placed in
 * position "index" (index values are from 0, 1, 2, ... for the first, second, third, ...) of the sequence
 * of corrections of the given type.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a the correction type: IONC_GAL, IONC_GPSA, IONC_GPSB, IONC_QZSA, IONC_QZSB, IONC_BDSA, IONC_BDSB, IONC_IRNA, IONC_IRNB or NOLABEL
 * @param b,the iono parameters or the time coeficients and reference
 * @param c the time mark for iono or UTC source id for time corrections
 * @param d the satellite number being source of data
 * @param index the position in the sequence of iono corrections records to get
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool  RinexData::getHdLnData(RINEXlabel rl, RINEXlabel &a, double (&b)[4], int &c, int &d, unsigned int index) {
//bool RinexData::getHdLnData(RINEXlabel rl, string &a, vector <double> &b, unsigned int index) {
    int order = -1;
    if (!getLabelFlag(rl)) return false;
    //convert V2 labels to equivalent V3 ones
    switch (rl) {
        case IONA:   //GPS iono alpha
            rl = IONC;
            a = IONC_GPSA;
            break;
        case IONB:    //GPS iono beta
            rl = IONC;
            a = IONC_GPSB;
            break;
		case DUTC:    //GPS UTC correction
			rl = TIMC;
			a = TIMC_GPUT;
			break;
		case CORRT:    //GLONASS UTC correction
			rl = TIMC;
			a = TIMC_GLUT;
			break;
		case GEOT:    //GEO UTC correction
			rl = TIMC;
			a = TIMC_SBUT;
			break;
        default:
            break;
    }
    switch(rl) {
        case IONC:
            //check if data requested exists; index is here the position in the list of a given type position
            for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); it++) {
                if ((it->corrType == a) || (a == NOLABEL)) order++;
                if (order == index) {
                    a = it->corrType;
                    for (int i = 0; i < 4; ++i) b[i] = it->corrValues[i];
                    c = (int) it->corrValues[4];
                    d = (int) it->corrValues[5];
                    return true;
                }
            }
            return false;
        default:
            throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
    }
}
/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - COMM to get from RINEX header the index-nd COMMENT data.
 *<p>Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a is the label identifier in the position following the comment
 * @param b the comment contents
 * @param index the position of the comment in the sequence of COMMENT records in the header.
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, RINEXlabel &a, string &b, unsigned int index) {
	switch(rl) {
	case COMM:
		for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it) {
			if(it->labelID == EOH) return false;
			if(it->hasData && (it->labelID == COMM)) {
				if (index == 0) {
					b = it->comment;
					it++;
					a = it->labelID;
					return true;
				} else index--;
			}
		}
		return false;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - PRNOBS to get from RINEX header the record data in index position of PRN / # OF OBS records.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a the system the satellite prn belongs
 * @param b the prn number of the satellite
 * @param c the number of observables for each observable type
 * @param index the position in the sequence of PRN / # OF OBS records to get.
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, char &a, int &b, vector <int> &c, unsigned int index) {
	switch(rl) {
	case PRNOBS:
		if (index < prnObsNum.size()) {
			GET_3PARAM(PRNOBS, prnObsNum[index].sysPrn, prnObsNum[index].satPrn, prnObsNum[index].obsNum)
		}
		return false;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - SCALE to get from RINEX header data the scale factor in index position of SYS / SCALE FACTOR records. Meaning of parametes is:
 *      a: is the system this scale factor has been applied,
 *      b: is a factor to divide stored observables with (1,10,100,1000),
 *      c: is the list of observable types involved. If vector is empty, all observable types are involved.
 *      index: is the position in the sequence of SYS / SCALE FACTOR records to get.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier 
 * @param b meaning depends on the label identifier
 * @param c meaning depends on the label identifier
 * @param index the position in the sequence of records the one to be extracted
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, char &a, int &b, vector <string> &c, unsigned int index) {
	switch(rl) {
	case SCALE:
		if (index < obsScaleFact.size()) {
			GET_3PARAM(SCALE, systems[obsScaleFact[index].sysIndex].system, obsScaleFact[index].factor, obsScaleFact[index].obsType)
		}
		return false;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - ANTPHC to get from RINEX header the average phase center position w/r to antenna reference point, which are included in ANTENNA: PHASECENTER. record.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a the system the satellite prn belongs
 * @param b the observable code
 * @param c the North (fixed station) or X coordinate in body-fixed coordinate system (vehicle)
 * @param d the East (fixed station) or Y coordinate in body-fixed coordinate system (vehicle)
 * @param e the Up (fixed station) or Z coordinate in body-fixed coordinate system (vehicle)
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, char &a, string &b, double &c, double &d, double &e) {
	switch(rl) {
	case ANTPHC:
		e = antPhUoZ;
		GET_4PARAM(ANTPHC,	antPhSys, antPhCode, antPhNoX, antPhEoY)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - DCBS to get from RINEX header the record data in index position of SYS / DCBS APPLIED records.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a the system the satellite prn belongs
 * @param b the program name used to apply corrections
 * @param c the source of corrections
 * @param index the position in the sequence of SYS / DCBS APPLIED records to get.
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, char &a, string &b, string &c, unsigned int index) {
	switch(rl) {
	case DCBS:
		if (index < prnObsNum.size()) {
			GET_3PARAM(PRNOBS, systems[dcbsApp[index].sysIndex].system, dcbsApp[index].corrProg, dcbsApp[index].corrSource)
		}
		return false;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - PHSH to get data from the given phshCorrection from "SYS / # / OBS TYPES" records
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a the system identification: G (GPS), R (GLONASS), S (SBAS), E (Galileo)
 * @param b a vector with identifiers for each observable type (C1C, L1C, D1C, S1C...) contained in epoch data for this system
 * @param index the position in the sequence of "# / TYPES OF OBSERV" or "SYS / # / OBS TYPES" records to get
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, char &a, string &b, double &c, vector<string> &d, unsigned int index) {
    vector<PHSHcorr>::iterator aPHSHit;
	switch(rl) {
	case PHSH:
		if (index < phshCorrection.size()) {
		    aPHSHit = phshCorrection.begin() + index;
			GET_4PARAM(SYS, systems[aPHSHit->sysIndex].system, aPHSHit->obsCode, aPHSHit->correction, aPHSHit->obsSats)
		}
		return false;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - SYS to get data from the given system index from "SYS / # / OBS TYPES" records
 * - TOBS to get data from the given system index from "# / TYPES OF OBSERV" records
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a the system identification: G (GPS), R (GLONASS), S (SBAS), E (Galileo), ...
 * @param b a vector with identifiers for each observable type (C1C, L1C, D1C, S1C...) contained in epoch data for this system
 * @param index the position in the sequence of "# / TYPES OF OBSERV" or "SYS / # / OBS TYPES" records to get
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, char &a,  vector <string> &b, unsigned int index) {
    vector <string> aVectorStr;
    switch(rl) {
        case SYS:
        case TOBS:
            if (index < systems.size()) {
                //fill the vector string with obsTypes identifiers selected
                for(vector<OBSmeta>::iterator it = systems[index].obsTypes.begin(); it < systems[index].obsTypes.end(); it++)
                    if (it->sel) aVectorStr.push_back(it->id);
                        GET_2PARAM(SYS, systems[index].system, aVectorStr)
            }
            return false;
        default:
            throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
    }
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - ANTZDAZI to get the azimuth of the zero-direction of a fixed antenna (degrees, from north). Data from record ANTENNA: ZERODIR AZI.
 * Param a is the value of azimuth of the zero-direction.
 * - INT to get from RINEX header the time interval of GPS measurements.
 * Param a is the value of the time interval in seconds.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier 
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, double &a) {
	switch(rl) {
	case ANTZDAZI:
		GET_1PARAM(ANTZDAZI, antZdAzi)
	case INT:
		GET_1PARAM(INT, obsInterval)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object.
 * TBD: change double &a by RINEXversion &ver
 * The label identifier values in this overload can be:
 * - VERSION to get the version (param a), file type (param b) and system type (param c) data from record RINEX VERSION / TYPE.
 * - INFILEVER to get the input file version (param a), file type (param b) and system type (param c) data read from an input RINEX file.
 * <p>Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier 
 * @param b meaning depends on the label identifier 
 * @param c meaning depends on the label identifier 
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, double &a, char &b, char &c) {
	switch(rl) {
	case VERSION:
		b = fileType;
		c = sysToPrintId;
		switch (version) {
		case V210: a = 2.10; break;
		case V304: a = 3.04; break;
		case VTBD: a = 0.0; break;
		default: return false;
		}
		return getLabelFlag(VERSION);
	case INFILEVER:
		b = fileType;
		c = sysToPrintId;
		switch (inFileVer) {
		case V210: a = 2.10; break;
		case V304: a = 3.04; break;
		case VTBD:
			a = 0.0;
		default:
			return false;
		}
		return true;
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - ANTHEN: to get antenna data from record "ANTENNA: DELTA H/E/N"
 * Param a is the antenna height (height of the antenna reference point (ARP) above the marker), param b is the antenna horizontal
 * eccentricity of ARP relative to the marker east, and param c is the antenna horizontal eccentricity of ARP relative to the marker north.
 * - APPXYZ: to get APROX POSITION record data.
 * Params a, b and c are respectively the X, Y and Z coordinates of the position.
 * - ANTXYZ: to get the position of antenna reference point for antenna on vehicle (m) from record ANTENNA: DELTA X/Y/Z.
 * Params a, b and c are respectively the X, Y and Z coordinates in body-fixed coordinate system.
 * - ANTBS: to set in RINEX header the direction of the �vertical� antenna axis towards the GNSS satellites. Data to be included in record ANTENNA: B.SIGHT XYZ.
 * Params a, b and c are respectively the X, Y and Z coordinates in body-fixed coordinate system.
 * - ANTZDXYZ: to set in RINEX header the zero-direction of antenna antenna on vehicle. Data to be included in record ANTENNA: ZERODIR XYZ.
 * Params a, b and c are respectively the X, Y and Z coordinates in body-fixed coordinate system.
 * - COFM: to set in RINEX header the current center of mass (X,Y,Z, meters) of vehicle in body-fixed coordinate system. Data to be included in record CENTER OF MASS: XYZ.
 * Params a, b and c are respectively the X, Y and Z coordinates in body-fixed coordinate system.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier 
 * @param b meaning depends on the label identifier 
 * @param c meaning depends on the label identifier 
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, double &a, double &b, double &c) {
	switch(rl) {
	case ANTHEN:
		GET_3PARAM(ANTHEN, antHigh, eccEast, eccNorth)
	case APPXYZ:
		GET_3PARAM(APPXYZ, aproxX, aproxY, aproxZ)
	case ANTXYZ:
		GET_3PARAM(ANTXYZ, antX, antY, antZ)
	case ANTBS:
		GET_3PARAM(ANTBS, antBoreX, antBoreY, antBoreZ)
	case ANTZDXYZ:
		GET_3PARAM(ANTZDXYZ, antZdX, antZdY, antZdZ)
	case COFM:
		GET_3PARAM(COFM, centerX, centerY, centerZ)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - CLKOFFS: to get in RINEX header if the realtime-derived receiver clock offset is applied or not. Data from record RCV CLOCK OFFS APPL.
 * Param a is the receiver clock applicability (1 or 0).
 * - LEAP to get from RINEX header the number of leap seconds since 6-Jan-1980 as transmitted by the GPS almanac. Data from record LEAP SECONDS.
 * Param a is the number of leap seconds.
 * - SATS to get from RINEX header the number of satellites, for which observables are stored in the file. Data from record # OF SATELLITES.
 * Param a is the number of satellites
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier 
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, int &a) {
	switch(rl) {
	case CLKOFFS:
		GET_1PARAM(CLKOFFS, rcvClkOffs)
	case LEAP:
		//to get values as per V210. GPS is allways the 1st element of the vector
		GET_1PARAM(LEAP, leapSecs[0].secs)
	case SATS:
		GET_1PARAM(SATS, numOfSat)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - GLSLT; to get Glonas slot - frequency number data
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a the wave length factor for L1
 * @param b the wave length factor for L2
 * @param index the position in the sequence of "WAVELENGTH FACT L1/2" records to get
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, int &a, int &b, unsigned int index) {
    switch(rl) {
        case GLSLT:
            if (index < gloSltFrq.size()) {
                GET_2PARAM(GLSLT, gloSltFrq[index].slot, gloSltFrq[index].frqNum)
            }
            return false;
        default:
            throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
    }
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - WVLEN to get a WAVELENGTH FACT L1/2 record in position index in the sequence of WAVELENGTH FACT L1/2 records. Meaning of parametes is:
 *      a: the wave length factor for L1
 *      b: the wave length factor for L2
 *      c: is a vector with satellite number (system + PRNs),
 *      index: is the position in the sequence of WAVELENGTH FACT L1/2 records to get
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier
 * @param b meaning depends on the label identifier
 * @param c meaning depends on the label identifier
 * @param index the position in the sequence of records the one to be extracted
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, int &a, int &b, vector <string> &c, unsigned int index) {
    switch(rl) {
        case WVLEN:
            if (index < wvlenFactor.size()) {
                GET_3PARAM(WVLEN, wvlenFactor[index].wvlenFactorL1, wvlenFactor[index].wvlenFactorL2, wvlenFactor[index].satNums)
            }
            return false;
        default:
            throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
    }
}

/**getHdLnData gets data values from RINEX file header records
 *
 * The label identifier values in this overload can be:
 * - LEAP to get from RINEX header the parameters related to leap seconds, being:
 *		a: value of leap seconds
 *		b: Future or past leap seconds ΔtLSF
 *		c: Respective week number WN_LSF
 *		d: Respective day number DN
 *		e: Time system identifier
 *		index: The position in the sequence of "LEAP SECONDS" records to get
 *
 * @param rl the label identifier of the RINEX header record/line data values are for
 * @param a meaning depends on the label identifier
 * @param b meaning depends on the label identifier
 * @param c meaning depends on the label identifier
 * @param d meaning depends on the label identifier
 * @param e meaning depends on the label identifier
 * @return true if header values have been set, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, int &a, int &b, int &c, int &d, char &e, unsigned int index) {
	switch(rl) {
		case LEAP:
			if (index < leapSecs.size()) {
			    e = leapSecs[0].sysId;
			    GET_4PARAM(LEAP, leapSecs[0].secs, leapSecs[0].deltaLSF, leapSecs[0].weekLSF, leapSecs[0].dayLSF)
			}
			return false;
		default:
			throw errorLabelMis + idTOlbl(rl) + msgSetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - SIGU: to get from RINEX header the unit of the signal strength observables Snn (if present). Data from record SIGNAL STRENGTH UNIT.
 * Param a is the unit of the signal strength.
 * - MRKNAME to get MARKER data for the RINEX file header. Data from record MARKER NAME.
 * Param a is the marker name.
 * - MRKNUMBER to get marker data from the RINEX file header. Data from record MARKER NUMBER.
 * Param a is the marker number.
 * - MRKTYPE to get marker data from the RINEX file header. Data from record MARKER TYPE records.
 * Param a is the marker type.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier 
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, string &a) {
	switch(rl) {
	case SIGU:
		GET_1PARAM(SIGU, signalUnit)
	case MRKNAME:
		GET_1PARAM(MRKNAME, markerName)
	case MRKNUMBER:
		GET_1PARAM(MRKNUMBER, markerNumber)
	case MRKTYPE:
		GET_1PARAM(MRKTYPE, markerType)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - AGENCY: to get OBSERVER / AGENCY record data of the RINEX file header.
 * Param a is the observer name, and param b is the agency name.
 * - ANTTYPE: to get antenna data from record "ANT # / TYPE".
 * Param a is the antenna number, and param b is the antenna type.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier 
 * @param b meaning depends on the label identifier 
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, string &a, string &b) {
	switch(rl) {
	case AGENCY:
		GET_2PARAM(AGENCY, observer, agency)
	case ANTTYPE:
		GET_2PARAM(ANTTYPE, antNumber, antType)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - RECEIVER: to get GNSS receiver data from the "REC # / TYPE / VERS" obligatory record in the RINEX file header.
 * Param a is the receiver number, param b is the receiver type, and param c is the receiver version.
 * - RUNBY: to get "PGM / RUN BY / DATE" data of the RINEX file header.
 * Param a is program used to create current file, param b is who executed the program, and param c is the date and time of file creation.
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier 
 * @param b meaning depends on the label identifier 
 * @param c meaning depends on the label identifier 
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, string &a, string &b, string &c) {
	switch(rl) {
	case RECEIVER:
		GET_3PARAM(RECEIVER, rxNumber, rxType, rxVersion)
	case RUNBY:
		GET_3PARAM(RUNBY, pgm, runby, date)
	default:
		throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}

/**getHdLnData gets data values related to line header records previously stored in the class object
 *
 * The label identifier values in this overload can be:
 * - GLPHS to get GLONASS phase bias correction used to align code and phase observations
 *		The meaning of parameters for this case hare:
 *		a the GLONASS signal identifier (C1C, C1P, C2C, C2P)
 *		b the code phase bias correction
 * <p> Values returned in parameters are undefined when method returns false.
 *
 * @param rl the label identifier of the RINEX header record/line data to be extracted
 * @param a meaning depends on the label identifier
 * @param b meaning depends on the label identifier
 * @return true if header values have been got, false otherwise
 * @throws error message when the label identifier value does not match the allowed params for this overload
 */
bool RinexData::getHdLnData(RINEXlabel rl, string &a, double &b, unsigned int index) {
	switch(rl) {
		case GLPHS:
			if (index < gloPhsBias.size()) {
				GET_2PARAM(GLPHS, gloPhsBias[index].obsCode, gloPhsBias[index].obsCodePhaseBias)
			}
			return false;
		default:
			throw errorLabelMis + idTOlbl(rl) + msgGetHdLn;
	}
}
#undef GET_1PARAM
#undef GET_2PARAM
#undef GET_3PARAM
#undef GET_4PARAM

/**lblTOid gives the label identification that best matches the label name passed (may be incomplete)
 * The label passed is searched in the table of labels. It is returned the identifier of the firts that
 * matches the string passed (its length may be shorter than the label name).
 *
 * @param label the label name to search in the table of label identifications
 * @return the label identification corresponding to the label name passed 
*/
RinexData::RINEXlabel RinexData::lblTOid(string label) {
	for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it)
		if (strncmp(label.c_str(), it->labelVal, label.size()) == 0) return it->labelID;
	return DONTMATCH;
}

/**idTOlbl gives the label name corresponding to the label identifier passed
 *
 * @param id the label identifier to search in the table of label identifications
 * @return the label name corresponding to the identifier passed, or an empty string if does not exist
*/
string RinexData::idTOlbl(RINEXlabel id) {
	for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it)
		if (it->labelID == id) return string(it->labelVal);
	return string();
}

/**get1stLabelId gives the first label identifier of the first record in the RINEX header having data (should be VERSION).
 *<p>A cursor is set to this first label to allow subsequent get operation.
 *<p>The get1stLabelId and getNextLabelId are useful to iterate over the header records having data set. Iteration would finish
 *when the RINEXlabel value returned is EOH or LASTONE.
 *
 * @return the label identifier of the first record having data, or LASTONE when all records are empty
*/
RinexData::RINEXlabel RinexData::get1stLabelId() {
	for (labelIdIdx = 0; labelIdIdx != labelDef.size(); labelIdIdx++) {
		if (labelDef[labelIdIdx].hasData) return labelDef[labelIdIdx].labelID;
	}
	return LASTONE;
}

/**getNextLabelId gives the label identifier of the next record in the RINEX header having data (after the one previously extracted).
 *<p>A cursor is set to this label to allow subsequent get operations.
 *<p>The get1stLabelId and getNextLabelId are useful to iterate over the header records having data set. Iteration would finish
 *when the RINEXlabel value returned is EOH or LASTONE.
 *
 * @return the label identifier of the next record having data, or LASTONE when there is not a next record having data
*/
RinexData::RINEXlabel RinexData::getNextLabelId() {
	while (++labelIdIdx < labelDef.size()) {
		if (labelDef[labelIdIdx].hasData) return labelDef[labelIdIdx].labelID;
	}
	return LASTONE;
}

/**clearHeaderData sets RINEX header state as "empty" for all its records (except EOH).
 *<p>Header records data shall be cleared before processing any spacial event epoch with header records, that is, special
 *events having flag values 2, 3, 4 or 5. The reason is that when printing such events, after the epoch line they are printed
 *all header line records having data.
 *<p>In sumary, to process special event it will be necessary to perform the following steps:
 * -# clearHeaderData before setting any setting of header record data. It is assumed the before doing this step RINEX headers have been printed
 * -# setHdLnData for all records to be included in the special event
 * -# setEpochTime stating the event flag value for the epoch to be printed
 * -# printObsEpoch to print RINEX epoch data
 */
void RinexData::clearHeaderData() {
	for (vector<LABELdata>::iterator it = labelDef.begin(); it != labelDef.end(); it++) it->hasData = false;
	wvlenFactor.clear();
	dcbsApp.clear();
	obsScaleFact.clear();
}

/*methods to process and collect current epoch data
*/
/**setEpochTime sets epoch time to the given week number and seconds (time of week), the receiver clock bias, and the epoch flag.
 * Flag values can be (see RINEX documents): 0: OK; 1: power failure between previous and current epoch; or >1: Special event.
 * Special events can be: 2: start moving antenna; 3: new site occupation (end of kinem. data, at least MARKER NAME record follows);
 * 4: header information follows; 5: external event (epoch is significant, same time frame as observation time tags)
 * 
 * @param weeks theweek number (from 01/06/1980)
 * @param secs the seconds from the beginning of the week
 * @param bias the receiver clock offset applied to epoch and observables (if any). By default is 0 sec.
 * @param eFlag the epoch flag stating the kind of data associted to this epoch (observables, special events, ...). By default is 0.
 * @return the GPS time set, in seconds from GPS ephemeris from the GPS ephemeris (6/1/1980)
 */
double RinexData::setEpochTime(int weeks, double secs, double bias, int eFlag) {
	epochWeek = weeks;
	epochTOW = secs;
	epochClkOffset = bias;
	epochFlag = eFlag;
	return getInstantGNSStime (epochWeek, epochTOW);
}

/**getEpochTime gets epoch time (week number, time of week), clock offset and event flag from the current epoch data.
 *<p>It also returns the GPS epoch time in seconds from the GPS ephemeris (6/1/1980).
 *<p>Flag values can be (see RINEX documents): 0: OK, 1: power failure between previous and current epoch, or >1: Special event.
 *<p>Special events can be: 2: start moving antenna, 3: new site occupation (end of kinem. data, at least MARKER NAME record follows),
 * 4: header information follows, 5: external event (epoch is significant, same time frame as observation time tags).
 * 
 * @param weeks the week number (from 01/06/1980)
 * @param secs the seconds from the beginning of the week
 * @param bias the receiver clock offset applied to epoch and observables (if any)
 * @param eFlag the epoch flag stating the kind of data associated to this epoch (observables, special events, ...)
 * @return the GPS time in seconds from the GPS ephemeris (6/1/1980)
 */
double RinexData::getEpochTime(int &weeks, double &secs, double &bias, int &eFlag) {
	weeks = epochWeek;
	secs = epochTOW;
	bias = epochClkOffset;
	eFlag = epochFlag;
	return getInstantGNSStime (epochWeek, epochTOW);
}

/**saveObsData stores measurement data for the given observable into the epoch data storage.
 * The given measurement data are stored only when they belong to the current epoch (same tTag as previous measurements
 * stored) or when the epoch data storage is empty, and if the given system and observable type belong to the ones to be stored,
 * that is, they are defined in the systems data for "SYS / # / OBS TYPES " or "# / TYPES OF OBSERV" records.
 * Observation data are time tagged (the tTag). The usual time tag is the TOW (real or estimated) when measurement was made.
 * For example, the time tag passed could be the TOW estimate made by the receiver before computing the solution.
 * All measurements belonging to the same epoch shall have identical time tag.
 * Note that true TOW is obtained when the receiver computes the position solution, providing it and/or a clock offset
 * with the difference between the estimated and true TOW. That is: GPS time = Estimated epoch time - receiver clock offset.
 *
 * @param sys the system identification (G, S, ...) the measurement belongs
 * @param sat the satellite PRN the measurement belongs
 * @param obsTp the type of observable/measurement (C1C, L1C, D1C, ...) as per RINEX V3.04
 * @param value the value of the measurement
 * @param lli the loss o lock indicator. See RINEX V2.10
 * @param strg the signal strength. See RINEX V3.04
 * @param tTag the time tag for the epoch this measurement belongs
 * @return true if data belong to the current epoch, false otherwise
 */
bool RinexData::saveObsData(char sys, int sat, string obsTp, double value, int lli, int strg, double tTag) {
    const string msgSysObs(" the system, in observable=");
	int sx = systemIndex(sys);	//system index
	if (epochObs.empty()) epochTimeTag = tTag;
	bool sameEpoch = epochTimeTag == tTag;
	//check if this observable type for this system shall be stored
	if (sameEpoch) {
		if (sx >= 0) {
			for (unsigned int ox = 0; ox < systems[sx].obsTypes.size(); ox++)
				if (obsTp.compare(systems[sx].obsTypes[ox].id) == 0) {
					epochObs.push_back(SatObsData(tTag, sx, sat, ox, value, lli, strg));
					return true;
				}
		}
		plog->warning(msgNotInSYS + msgSysObs + string(1,sys) + msgComma + obsTp);
	}
	return sameEpoch;
}

/**getObsData extract from current epoch storage observable data in the given index position.
 *
 * @param sys the system identification (G, S, ...) the measurement belongs
 * @param sat the satellite PRN the measurement belongs
 * @param obsTp the type of observable/measurement (C1C, L1C, D1C, ...) as per RINEX V3.04
 * @param value the value of the measurement
 * @param lli the loss of lock indicator. See RINEX V2.10
 * @param strg the signal strength. See RINEX V3.04
 * @param index the position in the sequence of opoch observables to extract
 * @return true if data for the given index exist, false otherwise
 */
bool RinexData::getObsData(char &sys, int &sat, string &obsTp, double &value, int &lli, int &strg, unsigned int index) {
	if (epochObs.size() <= index) return false;
	vector<SatObsData>::iterator it = epochObs.begin() + index;
	sys = systems[it->sysIndex].system;
	sat = it->satellite;
	obsTp = systems[it->sysIndex].obsTypes[it->obsTypeIndex].id;
	value = it->obsValue;
	lli = it->lossOfLock;
	strg = it->strength;
	return true;
}

/**setFilter set the selected values for systems, satellites and observables to filter header, observation and navigation data.
 * Filtering data are reset (to 'no filter') and values passed (if any) will be used to filter epoch data in the following way:
 * - an empty list of selected satellites means that data from all satellites will pass the filter.
 * - a non-empty list of selected satellites states the ones that will pass the filter. Data belonging to satellites not in the list will not pass the filter. 
 * - an empty list of selected observables means that no changes will be made on the current selection
 * - a non-empty list of selected observables states the ones that will pass the filter. Data belonging to other observables will not pass the filter.
 * The filtering values set will be used by the methods filterObsData and filterNavData to remove observation and navigation data not selected from a RinexData object.
 * The method verifies coherence of the given data with regard to what has been defined in header records:
 * - a selected system shall be in the SYS / # / OBS TYPES records (or similar information for RINEX 2.10)
 * - the system identifier in a selected satellite shall be also in the header system definition records
 * - the system identifier in a selected observable shall be also in the header system definition records, and the observable type shall be in the list of abservables for this system
 * Note that pure navigation RINEX header data do not allow to verify such coherence, therefore the result of such coherence analysis is not relevant when setting
 * for filtering navigation data.
 *
 * @param selSat the vector containing the list of selected systems and / or satellites. A system is identified by its assigned char (G, E, R,...). A satellite is identified by the system char followed by the prn number.
 * @param selObs the vector containing the list of selected observables. An observable is identified by the system identification char (G, E, R, ...) followed by the observation code as defined in RINEX v3.04
 * @return true when filtering data are coherent or not filtering is requested, false when filtering data are not coherent
 */
bool RinexData::setFilter(vector<string> selSat, vector<string> selObs) {
#define SET_OBS_SELECTED \
    for (obsIdx = 0; obsIdx < systems[sysIdx].obsTypes.size(); obsIdx++) { \
        if (systems[sysIdx].obsTypes[obsIdx].id.compare((*itSelObs).substr(1)) == 0) { \
            selectedObs.push_back(SELobs(sysIdx, obsIdx)); \
            found = true; \
            break; \
        } \
    }

    const string msgFilterCleared("Filtering data not stated");
    const string msgWrongSysSat("Wrong sys-sat format (Ignored for filtering)=");
    const string msgSelObs(" the selected observable=");
    const string msgNoSel("Not selected system:");
    const string msgFilterStated("Filtering data stated: ");
    const string msgSelSys("Selected system satellites, observables: ");
    struct SELsats {    //to store selSat data after verification
        int sysIndex;
        int satNumber;
        //constructor
        SELsats(int si, int ns) {
            sysIndex = si;
            satNumber = ns;
        }
    };
    vector<SELsats> selectedSats;
    struct SELobs {     //to store selObs after verification
        int sysIndex;
        int obsIndex;
        //Constructor
        SELobs(int si, int oi) {
            sysIndex = si;
            obsIndex = oi;
        }
    };
    vector<SELobs> selectedObs;
    vector<GNSSsystem>::iterator itsys;
    vector<int> inxSysObs;
    vector<int> inxObsSys;
    char s, b[5];
    int sysIdx, obsIdx, n, o;
    bool found;
    string aStr;
    plog->info(msgFilterStated);
    //1st: Verify input data in selSat. Save system identification and satellite number of correct ones
    bool areCoherent = true;
    for (vector<string>::iterator it = selSat.begin(); it != selSat.end(); it++) {
        switch (sscanf((*it).c_str(), "%c%d", &s, &n)) {
            case 1:
                if ((sysIdx = systemIndex(s)) >= 0) selectedSats.push_back(SELsats(sysIdx, -1));
                break;
            case 2:
                if ((sysIdx = systemIndex(s)) >= 0) selectedSats.push_back(SELsats(sysIdx, n));
                break;
            default:
                sysIdx = -1;
        }
        if (sysIdx < 0) {
            plog->warning(msgWrongSysSat + (*it));
            areCoherent = false;
        }
    }
    //verify given data for selected systems - observations. Save system index and observation index of correct ones
    for (vector<string>::iterator itSelObs = selObs.begin(); itSelObs != selObs.end(); itSelObs++) {
        found = false;
        if ((sysIdx = systemIndex((*itSelObs).at(0))) >= 0) {
            SET_OBS_SELECTED
        } else if ((*itSelObs).at(0) == 'M') {
            for (sysIdx = 0; sysIdx < systems.size(); sysIdx++) {
                SET_OBS_SELECTED
            }
        }
        if (!found) {
            plog->warning(msgNotInSYS + msgSelObs + (*itSelObs));
            areCoherent = false;
        }
    }
    if (selectedSats.empty()) {
        //reset system status as ALL SYSTEMS AND SATELLITES SELECTED
        for (vector<GNSSsystem>::iterator it = systems.begin(); it != systems.end(); it++) {
            it->selSystem = true;
            it->selSat.clear();
        }
    } else {
        //there is at least a system selected to filter data. Reset select data in systems
        for (vector<GNSSsystem>::iterator it = systems.begin(); it != systems.end(); it++) {
            it->selSystem = false;
            it->selSat.clear();
        }
        //update select data in systems for satelite selected
        for (vector<SELsats>::iterator it = selectedSats.begin(); it!= selectedSats.end(); it++) {
            systems[it->sysIndex].selSystem = true;
            if (it->satNumber != -1) systems[it->sysIndex].selSat.push_back(it->satNumber);
        }
    }
    //update observation data in systems for observations selected
    if (!selectedObs.empty()) {
        for (vector<GNSSsystem>::iterator it = systems.begin(); it != systems.end(); it++) {
            it->selSystem = false;
            for (vector<OBSmeta>::iterator itobs = it->obsTypes.begin(); itobs != it->obsTypes.end(); itobs++) itobs->sel = false;
        }
        for (vector<SELobs>::iterator it = selectedObs.begin(); it != selectedObs.end(); it++) {
            systems[it->sysIndex].selSystem = true;
            systems[it->sysIndex].obsTypes[it->obsIndex].sel = true;
        }
    }
    for (vector<GNSSsystem>::iterator it = systems.begin(); it != systems.end(); it++) {
        if (it->selSystem) {
            aStr = msgSelSys + string(1, it->system) + msgComma;
            for (vector<int>::iterator itsat = it->selSat.begin(); itsat != it->selSat.end(); itsat++) {
                aStr += msgSpace + to_string(*itsat);
            }
            aStr += msgComma;
            for (vector<OBSmeta>::iterator itobs = it->obsTypes.begin(); itobs != it->obsTypes.end(); itobs++) {
                if (itobs->sel) aStr += msgSpace + itobs->id;
            }
        } else {
            aStr = msgNoSel + string(1, it->system);
        }
        plog->info(aStr);
    }
    return areCoherent;
#undef SET_OBS_SELECTED
}

/**filterObsData if filtering data have been stated using setFilter method, removes from current epoch observation data on systems, satellites or observables not selected.
 *
 * @param removeNotPrt when true they are removed also the observables not to be printed
 * @return true when it remains any epoch data after filtering, false when no data remain
 */
bool RinexData::filterObsData(bool removeNotPrt) {
	vector<SatObsData>::iterator it;
	it = epochObs.begin();
	while (it != epochObs.end()) {
        //check if its system, observable or satellite is not selected, or if requested, the observable will not be printed
		if (!systems[it->sysIndex].selSystem ||
					!systems[it->sysIndex].obsTypes[it->obsTypeIndex].sel ||
					!isSatSelected(it->sysIndex, it->satellite) ||
                    (removeNotPrt && !systems[it->sysIndex].obsTypes[it->obsTypeIndex].prt)) {
			it = epochObs.erase(it);
		} else it++;
	}
	return !epochObs.empty();
}

/**clearObsData clears all epoch observation data on satellites and observables previously saved.
 *
 */
void RinexData::clearObsData() {
	epochObs.clear();
}

/**saveNavData stores navigation data from a given satellite into the navigation data storage.
 * Only new epoch data are stored: tTag, system and satellite shall be different from other records already saved.
 *
 * @param sys the satellite system identifier (G,E,R, ...)
 * @param sat the satellite PRN the navigation data belongs
 * @param tTag the time tag for the navigation data
 * @param bo the broadcast orbit data with the eight lines of RINEX navigation data with four parameters each
 * @return true if data have been saved, false otherwise
 */
bool RinexData::saveNavData(char sys, int sat, double bo[BO_MAXLINS][BO_MAXCOLS], double tTag) {
	//check if this sat epoch data already exists: same satellite and time tag
	string logmsg = msgEpheSat+ string(1,sys) + to_string(sat) + msgTimeTag + to_string(tTag);
	for (vector<SatNavData>::iterator it = epochNav.begin(); it != epochNav.end(); it++) {
		if((sys == it->systemId) && (sat == it->satellite) && (tTag == it->navTimeTag)) {
			plog->fine(logmsg + msgAlrEx);
			return false;
		}
	}
	try {
		epochNav.push_back(SatNavData(tTag, sys, sat, bo));
		plog->fine(logmsg + msgSaved);
	} catch (std::bad_alloc& ba) {
		plog->warning(logmsg + msgNoMem + ba.what());
	}
	return true;
}

/**getNavData extract from current epoch storage navigation data in the given index position.
 *
 * @param sys the satellite system identifier (G,E,R, ...)
 * @param sat the satellite PRN the navigation data belongs
 * @param tTag the time tag for the navigation data
 * @param bo the broadcast orbit data with the eight lines of RINEX navigation data with four parameters each
 * @param index the position in the sequence of epoch records to get
 * @return true if data have been saved, false otherwise
 */
bool RinexData::getNavData(char& sys, int &sat, double (&bo)[BO_MAXLINS][BO_MAXCOLS], double &tTag, unsigned int index) {
	if (epochNav.size() <= index) return false;
	vector<SatNavData>::iterator it = epochNav.begin() + index;
	sys = it->systemId;
	sat = it->satellite;
	for (int i = 0; i < BO_MAXLINS; i++)
		for (int j = 0; j < BO_MAXCOLS; j++)
			bo[i][j] = it->broadcastOrbit[i][j];
	tTag = it->navTimeTag;
	return true;
}

/**filterNavData if filtering data have been stated using setFilter method, removes from current epoch data on system or satellites not selected.
 *
 * @return true when it remains any epoch data after filtering, false when no data remain
 */
bool RinexData::filterNavData() {
	vector<SatNavData>::iterator it;
	it = epochNav.begin();
	while (it != epochNav.end()) {
        if (!isSatSelected(systemIndex(it->systemId), it->satellite)) it = epochNav.erase(it);
        else it++;
	}
	return !epochNav.empty();
}

/**clearNavData clears all epoch navigation data on satellites and ephemeris previously saved.
 *
 */
void RinexData::clearNavData() {
	epochNav.clear();
}

/**getObsFileName constructs a standard RINEX observation file name from the given prefix and current header data.
 * For V2.1 RINEX file names, the given prefix and the current TIME OF FIRST OBSERVATION header data are used.
 * Additionally, for V3.04 the file name includes data from MARKER NUMBER, REC # / TYPE / VERS, TIME OF FIRST OBS and TIME OF LAST OBS,
 * INTERVAL, and SYS / # / OBS TYPES header records (if defin3ed) and country parameter.
 *
 * @param prefix : the file name prefix
 * @param country the 3-char ISO 3166-1 country code, or "---" if parameter not given
 * @return the RINEX observation file name in the standard format (PRFXdddamm.yyO for v2.1, XXXXMRCCC_R_YYYYDDDHHMM_FPU_DFU_DO.RNX for v3.04)
 */
string RinexData::getObsFileName(string prefix, string country) {
    try {
        setFileDataType('O');
    } catch (string errorMsg) {
        plog->warning(msgBadFileName + errorMsg);
        return "BadObsName.txt";
    }
	switch(version) {
	case V304:
		return fmtRINEXv3name(prefix, firstObsWeek, firstObsTOW, country);
	default:
		return fmtRINEXv2name(prefix, firstObsWeek, firstObsTOW);
	}
}

/**getNavFileName constructs a standard RINEX navigation file name from the given prefix and using current header data.
 * For V2.1 RINEX file names, the given prefix and suffix parameters data are used.
 * Additionally, for V3.04 the file name includes data from MARKER NUMBER, REC # / TYPE / VERS, TIME OF FIRST OBS and TIME OF LAST OBS,
 * and SYS / # / OBS TYPES header records (if defin3ed) and country parameter.
 *
 * @param prefix the file name prefix as 4-character station name designator
 * @param country the 3-char ISO 3166-1 country code, or "---" by default
 * @return the RINEX file name in the standard format  (PRFXdddamm.yyN for v2.1, XXXXMRCCC_R_YYYYDDDHHMM_FPU_DFU_DN.RNX for v3.04)
 * @throws error message string when a navigation file name cannot be
 */
string RinexData::getNavFileName(string prefix, string country) {
    try {
        setFileDataType('N');
    } catch (string errorMsg) {
        plog->warning(msgBadFileName + errorMsg);
        return "BadNavName.txt";
    }
	int week = epochWeek;
	double tow = epochTOW;
	if (getLabelFlag(TOFO)) {
		week = firstObsWeek;
		tow = firstObsTOW;
	}
	if (!epochNav.empty()) {
		sort(epochNav.begin(), epochNav.end());
		week = getWeekGNSSinstant(epochNav[0].navTimeTag);
		tow = getTowGNSSinstant(epochNav[0].navTimeTag);
	}
	switch(version) {
	case V304:
		return fmtRINEXv3name(prefix, week, tow, country);
	default:
		return fmtRINEXv2name(prefix, week, tow);
	}
}

/**printObsHeader prints the RINEX observation file header using data stored for header records.
 * At least, a system has to be previously selected for being printed.
 *
 * @param out the already open print stream where RINEX header will be printed
 * @throws error message string when header cannot be printed
 */
void RinexData::printObsHeader(FILE* out) {
	///Before printing, set and verify VERSION data record:
	if (version == VTBD) version = inFileVer;
	if (version == VTBD) throw msgVerTBD;
	/// - Set file type for Observation.
    try {
        setFileDataType('O', true);
        setSuffixes();
    } catch (string errorMsg) {
        plog->warning(msgWrongVer + errorMsg);
    }
	/// - Set the system identification for the one to be printed.
	setLabelFlag(VERSION);
	/// - Depending on version to be printed, set "# / TYPES OF OBSERV" or "SYS / # / OBS TYPES" data record.
	if(version == V210) {
        //in V210 all systems shall have the same observables to print
        //compute aVectorBool setting to true the corresponding obsTypes that shall be printed
        //Note that only V210 obsTypes are taken into account
        vector<bool> aVectorBool;
        aVectorBool.insert(aVectorBool.begin(), numberV2ObsTypes, false);
        for (vector<GNSSsystem>::iterator itsys = systems.begin(); itsys != systems.end(); itsys++) {
            for (int i = 0; i < numberV2ObsTypes; i++) {
                itsys->obsTypes[i].prt = itsys->obsTypes[i].sel;
                aVectorBool[i] = aVectorBool[i] || itsys->obsTypes[i].prt;
            }
        }
        //set in systems the obTypes data related to printing
        bool isAny;
        for (vector<GNSSsystem>::iterator itsys = systems.begin(); itsys != systems.end(); itsys++) {
            //check if this system has any obsType to print
            isAny = false;
            for (int i = 0; i < numberV2ObsTypes; i++) if (itsys->obsTypes[i].prt) { isAny = true; break; }
            if (isAny) {
                //obsType shall be printed using a common pattern for all systems
                for (int i = 0; i < numberV2ObsTypes; i++) itsys->obsTypes[i].prt = aVectorBool[i];
            }
            for (int i = numberV2ObsTypes; i < itsys->obsTypes.size(); i++) itsys->obsTypes[i].prt = false;
        }
		setLabelFlag(SYS, false);
		setLabelFlag(TOBS);
	} else {	//version will be V304
        //determine de obsTypes to print in this version (all selected)
        for (vector<GNSSsystem>::iterator itsys = systems.begin(); itsys != systems.end(); itsys++) {
            for (vector<OBSmeta>::iterator itobs = itsys->obsTypes.begin(); itobs != itsys->obsTypes.end(); itobs++) {
                itobs->prt = itobs->sel;
            }
        }
		setLabelFlag(SYS);
		setLabelFlag(TOBS, false);
	}
    setLabelFlag(EOH);	//END OF HEADER record shall allways be printed
	///Finally, for each observation header record belonging to the current version and having data defined, print it.
	for (vector<LABELdata>::iterator it = labelDef.begin(); it != labelDef.end(); it++) {
		if (((it->type & OBSMSK) != OBSNAP) && (it->ver == VALL || it->ver == version)) {
			if (it->hasData) printHdLineData(out, it);
            ///Log a warning message when the record to be printed is obligatory, but has not data.
			else if ((it->type & OBSMSK) == OBSOBL) plog->warning(valueLabel(it->labelID, msgHdRecNoData));
		}
	}
}

/**printObsEpoch prints the data lines for one epoch using the current stored observation data.
 *Its is assumed that all observation data currently stored belong to the same epoch.
 *The process to print the current epoch data takes into account:
 * - The version of the observation file to be printed, which implies to use different formats.
 * - The kind of epoch data to be printed: epoch with observable data, cycle slip records, or event flag records.
 * - If continuation lines shall be used or not, depending on version and amount of data to be printed
 *<p>Data filtering is performed before printing (when data filters have been set).
 *<p>Observation data are removed from storage after printing them, but for special event epochs their special
 *records (header lines) are not cleared after printing them.
 * 
 * @param out the already open print stream where RINEX epoch data will be printed
 * @throws error message string when epoch data cannot be printed due to undefined version to be printed
 */
void RinexData::printObsEpoch(FILE* out) {
///a macro to compute comparison expression of satellite in two consecutive observables, in iterator POSITION and (POSITION-1)
	#define DIFFERENT_SAT(POSITION) \
		((POSITION-1)->sysIndex != POSITION->sysIndex) ||	\
		((POSITION-1)->satellite != POSITION->satellite)

	char timeBuffer[80], clkOffsetBuffer[20];
	vector<SatObsData>::iterator it;
	int anInt;
	bool clkOffsetPrinted = false;	//a flag to know if clock offset has been printed or not
	clkOffsetBuffer[0] = 0;		//set an empty string in the buffer
	//set the printable epoch time and clock offset using format of the version to be printed.
	switch (version) {
	case V210:	//RINEX version 2.10
		formatGPStime (timeBuffer, 80, " %y %m %d %H %M", "%11.7f", epochWeek, epochTOW);
		if((epochClkOffset < 99.999999999) && (epochClkOffset > -9.999999999)) sprintf(clkOffsetBuffer, "%12.9f", epochClkOffset);
		break;
	case V304:	//RINEX version 3.04
		formatGPStime (timeBuffer, 80, "> %Y %m %d %H %M", "%11.7f", epochWeek, epochTOW);
		if((epochClkOffset < 99.999999999999) && (epochClkOffset > -9.999999999999)) sprintf(clkOffsetBuffer, "%15.12f", epochClkOffset);
		break;
	default:
		throw string(msgVerTBD);
	}
	switch (epochFlag) {
	case 0:	//epoch with observable data OK
	case 1:	//power failure between previous and current epoch
	case 6:	//cycle slip records follow (same format as per observables)
		// If data filtering was requested, remove observation data not selected.
		// Even the whole epoch could be removed if it is outside of a selected time period.
		// Ends if it does not remain any data to print.
		if (!filterObsData(true)) return;
        stable_sort(epochObs.begin(), epochObs.end());
        //count the number of different satellites with data in this epoch (at least one)
        nSatsEpoch = 1;
        for (it = epochObs.begin()+1; it != epochObs.end(); it++) if (DIFFERENT_SAT(it)) nSatsEpoch++;
		switch (version) {
		case V210:	//RINEX version 2.10
            //start printing epoch 1st line
	 		fprintf(out, "%s  %1d%3d", timeBuffer, epochFlag, nSatsEpoch);
			//append the different systems and satellites existing in this epoch.
			//if number of satellites is greather than 12, use continuation lines. Clock offset is printed only in the 1st one
			fprintf(out, "%1c%02d", systems[epochObs[0].sysIndex].system, epochObs[0].satellite);
			anInt = 1;		//currently, the number of satellites already printed
			for (it = epochObs.begin()+1; it != epochObs.end(); it++)
				if (DIFFERENT_SAT(it)) {
					if ((anInt % 12) == 0) fprintf(out, "\n%32c", ' '); //print the begining of a continuation line
					fprintf(out, "%1c%02d", systems[it->sysIndex].system, it->satellite);
					anInt++;
					if (anInt == 12) {		//printed last sat in the 1st line
						fprintf(out, "%s", clkOffsetBuffer);
						clkOffsetPrinted = true;
					}
				}
			while ((anInt % 12) != 0) {	//fill the line
				fprintf(out, "%3c", ' ');
				anInt++;
			}
			if (clkOffsetPrinted) fprintf(out, "\n");
			else fprintf(out, "%s\n", clkOffsetBuffer);
			//for each satellite in this epoch, print their observables and remove them from epochObs
			while (printSatObsValues(out, V210))
			    ;
	 		break;
		case V304:	//RINEX version 3.04
            fprintf(out, "%s  %1d%3d%5c%s%3c\n", timeBuffer, epochFlag, nSatsEpoch, ' ', clkOffsetBuffer, ' ');
			//for each satellite in this epoch,  print a line with their measurements (they are removed just after printed)
			do {
				fprintf(out, "%1c%02d", systems[epochObs[0].sysIndex].system, epochObs[0].satellite);
 			} while (printSatObsValues(out, V304));
 			break;
		default:
		     break;
 		}
		break;
	case 2:	//start moving antenna event
	case 3:	//new site occupation event
	case 4:	//header information event
    case 5:	//external event
		//count the number of special records (header lines) to print
		nSatsEpoch = 0;
		for (vector<LABELdata>::iterator lit = labelDef.begin(); lit != labelDef.end(); lit++) {
			if (lit->hasData && ((lit->type & OBSMSK) != OBSNAP) && (lit->ver == VALL || lit->ver == version))
				nSatsEpoch++;
		}
		//print epoch 1st line. Note that nSatsEpoch contains the number of special records that follow
 		fprintf(out, "%s  %1d%3d\n", timeBuffer, epochFlag, nSatsEpoch);
		if (nSatsEpoch > 0) {
			//print the header lines that follow
			for (vector<LABELdata>::iterator lit = labelDef.begin(); lit != labelDef.end(); lit++) {
                //*if (lit->hasData && lit->labelID != EOH && ((lit->type & OBSMSK) != OBSNAP) && (lit->ver == VALL || lit->ver == version))
				if (lit->hasData && ((lit->type & OBSMSK) != OBSNAP) && (lit->ver == VALL || lit->ver == version))
					printHdLineData(out, lit);
			}
		}
		break;
	}
#undef DIFFERENT_SAT
}

/**printEndOfFile prints the RINEX end of file event lines.
 *
 * @param out	The already open print file where RINEX data will be printed
 */
 void RinexData::printObsEOF(FILE* out) {
	 //set data to create an event record "header information follows"
	 epochFlag = 4;
	 clearHeaderData();
	 setHdLnData(COMM, LASTONE, string("END OF FILE"));
	 printObsEpoch(out);
}
 
 /**printNavHeader prints RINEX navigation file header using the current RINEX data.
 * 
 * @param out	The already open print file where RINEX header will be printed
 * @throws error message string when header cannot be printed
 */
void RinexData::printNavHeader(FILE* out) {
	///Before printing, set VERSION data record which depends on the version to be printed.
	const string msgNotNav("No system selected to generate navigation file");
	int n = 0;
	if (version == VTBD) version = inFileVer;
	if (version == VTBD) throw msgVerTBD;
    try {
         setFileDataType('N', true);
         setSuffixes();
    } catch (string errorMsg) {
         plog->warning(msgWrongVer + errorMsg);
    }
	setLabelFlag(VERSION);
    setLabelFlag(EOH);	//END OF HEADER record shall allways be printed
	///Finally, for each navigation header record belonging to the current version and having data defined, it is printed.
	for (vector<LABELdata>::iterator it = labelDef.begin(); it != labelDef.end(); it++) {
		if (((it->type & NAVMSK) != NAVNAP) && (it->ver == VALL || it->ver == version)) {
			if (it->hasData)
				printHdLineData(out, it);
			else if ((it->type & NAVMSK) == NAVOBL)
				///Log a warning message when the record to be printed is obligatory, but has not data.
				plog->warning(valueLabel(it->labelID, msgHdRecNoData));
		}
	}
}

/**printNavEpochs prints ephemeris data stored according version and systems selected.
 *<p>If version to print is V3.04, all stored data are printed, but if version to print is V2.10
 *only data for the system stated in the version header record will be printed, which is the first
 *in the list of selected systems (using setFilter method).
 *<p>Ephemeris data printed are removed from the storage.
 *<p>This method does not removes data from non-selected systems-satellites.
 * 
 * @param out the already open print file where RINEX epoch will be printed
 * @throws error message string when epoch cannot be printed
 */
void RinexData::printNavEpochs(FILE* out) {
    const string msgNavEpochsSys("Navigation epochs for system=");
    const string msgNavEpochIgn("Ignored epoch for system, satellite=");
    const string msgNavEpochPrn("Printed epoch for system, satellite=");
	char timeBuffer[80];
	int nBroadcastOrbits, nEphemeris;
	const char* timeFormat;
	const char* secondsFormat;
	int lineStartSpaces;
	vector<SatNavData>::iterator it;

#ifdef _WIN32
	//MS VS specific!!
	_set_output_format(_TWO_DIGIT_EXPONENT);
#endif
	if(epochNav.empty()) return;
	//set version constants
	switch (version) {
	case V210:
		timeFormat = "%y %m %d %H %M";
		secondsFormat = " %4.1f";
		lineStartSpaces = 3;
		break;
	case V304:
		timeFormat = "%Y %m %d %H %M %S";
		secondsFormat = "";
		lineStartSpaces = 4;
		break;
	default:
		throw msgVerTBD;
	}
	//filter nav epochs available
	//if (!filterNavData()) return;
	//sort epochs available by time tag, system, and satellite
	sort(epochNav.begin(), epochNav.end());
	plog->finest(msgNavEpochsSys + string(1, sysToPrintId) + msgColon);
	for (vector<SatNavData>::iterator it = epochNav.begin(); it != epochNav.end(); it++) {
	    if (isSatSelected(systemIndex(it->systemId), it->satellite)) {
            plog->finest(msgNavEpochPrn + string(1, it->systemId) + msgComma + to_string(it->satellite));
            //print epoch first line
            formatGPStime (timeBuffer, sizeof timeBuffer, timeFormat, secondsFormat, getWeekGNSSinstant(it->navTimeTag), getTowGNSSinstant(it->navTimeTag));
            switch (version) {	//print satellite and epoch time
                case V210:
                    fprintf(out, "%02d %s", it->satellite, timeBuffer);
                    if (it->systemId == 'R') {	//in V2 GLONASS tk to print is daily, not weekly
                        it->broadcastOrbit[0][3] = fmod(it->broadcastOrbit[0][3], 86400);
                    }
                    break;
                case V304:
                    fprintf(out, "%1c%02d %s", it->systemId, it->satellite, timeBuffer);
                    break;
                default:
                    break;
            }
            for (int i=1; i<BO_MAXCOLS; i++)	//add the Af0, Af1 & Af2 values
                fprintf(out, "%19.12E", it->broadcastOrbit[0][i]);
            fprintf(out, "\n");
            //print the rest of broadcast orbit data lines
            switch (it->systemId) {
                //set values for nBroadcastOrbits and nEphemeris as stated in RINEX 3.04 doc
                case 'G': nBroadcastOrbits = BO_MAXLINS_GPS; nEphemeris = BO_TOTEPHE_GPS; break;
                case 'R': nBroadcastOrbits = BO_MAXLINS_GLO; nEphemeris = BO_TOTEPHE_GLO; break;
                case 'E': nBroadcastOrbits = BO_MAXLINS_GAL; nEphemeris = BO_TOTEPHE_GAL; break;
				case 'C': nBroadcastOrbits = BO_MAXLINS_BDS; nEphemeris = BO_TOTEPHE_BDS; break;
                case 'S': nBroadcastOrbits = BO_MAXLINS_SBAS; nEphemeris = BO_TOTEPHE_SBAS; break;
                default: throw msgSysUnk + string(1, it->systemId);
            }
            for (int i = 1; (i < nBroadcastOrbits) && (nEphemeris > 0); i++) {
                for (int j = 0; j < lineStartSpaces; j++) fputc(' ', out);
                for (int j = 0; j < BO_MAXCOLS; j++) {
                    if (nEphemeris > 0) fprintf(out, "%19.12E", it->broadcastOrbit[i][j]);
                    else fprintf(out, "%19c", ' ');
                    nEphemeris--;
                }
                fprintf(out, "\n");
            }
	    } else {
            plog->finest(msgNavEpochIgn + string(1,it->systemId) + msgComma + to_string(it->satellite));
	    }
	}
}

/**
 * hasNavEpochs checks if it exists navigation data saved in the RINEX object for the given constellation.
 *
 * @param sys the constellation identification
 * @return true if it exists navigation data for the given constellation, false otherwise
 */
bool RinexData::hasNavEpochs(char sys) {
    vector<SatNavData>::iterator it = epochNav.begin();
    while (it != epochNav.end()) {
        if (it->systemId == sys) return true;
        it++;
    }
    return false;
}

/**readRinexHeader read the RINEX file header extracting its data and storing them into to the class members.
 * As a well formed RINEX head shall terminate with the "END OF HEADER" line, the normal return for the method would be EOH.
 * If the order of the header records is not compliant with what is stated in the RINEX definition, the error is logged.
 * Format errors in line/record data are logged, but do not interrupt reading. It is out of the scope of this method verify data read.
 * Process terminates when: end of header line is read, EOF is found in the input file, or they are read ten lines without label (may be a header without EOH).
 *
 * @param input the already open print stream where RINEX header will be read
 * @return EOH if end of header line is read, LASTONE if EOF is found, or NOLABEL if at least ten lines without label are read
 */
RinexData::RINEXlabel RinexData::readRinexHeader(FILE* input) {
    const string msgReadFileHd("Data read from RINEX file header:");
    const string msgLabelErr(" label error");
    const string msgLabel1st(" shall follow VERSION");
    const string msgLabelOrder1(" shall be preceded by SYS");
    const string msgLabelOrder2(" shall be preceded by SATS");
    const string msgLabelRepeted(" cannot appear twice");
    const string msgNotFnd(" not found");
	RINEXlabel labelId;
	int maxErrors = 10;
	plog->fine(msgReadFileHd);
	//state variable to check correct order of header lines. Values are:
	// 0 : No lines read. VERSION shall follow
	// 1 : VERSION line read. No lines labeled DCBS, SCALE, or PRN can immediately follow
	// 2 : SYS read. DCBS, SCALE can follow, but no PRN can immediately follow
	// 3 : SATS read. PRN can follow
	// 4 : EOH read
	int lineOrder = 0;
	//read lines from input file
	do {
		labelId = readHdLineData(input);
		//verify order in line read according state in lineOrder, and modify state accordingly
		switch (labelId) {
		case NOLABEL:
			maxErrors--;
		case DONTMATCH:
			plog->warning(valueLabel(labelId, msgLabelErr));
		case LASTONE:
			break;
		default:
			switch (lineOrder) {
			case 0:	//VERSION shall be the first line
				if (labelId == VERSION) {
					if (getLabelFlag(VERSION)) lineOrder = 1;
					else return VERSION;
				}
				else plog->warning(valueLabel(labelId, msgLabel1st));
				break;
			case 1:	//VERSION line received. Any label except DCBS, SCALE, or PRN can follow
				switch (labelId) {
				case VERSION: plog->warning(valueLabel(labelId, msgLabelRepeted)); break;
				case DCBS:
				case SCALE: plog->warning(valueLabel(labelId, msgLabelOrder1)); break;
				case PRNOBS: plog->warning(valueLabel(labelId, msgLabelOrder2)); break;
				case SYS: lineOrder = 2; break;
				case SATS: lineOrder = 3; break;
				case EOH: lineOrder = 4;
				default: break;
				}
				break;
			case 2:	//SYS received. DCBS, SCALE can follow.
				switch(labelId) {
				case VERSION: plog->warning(valueLabel(labelId, msgLabelRepeted)); break;
				case PRNOBS: plog->warning(valueLabel(labelId, msgLabelOrder2)); break;
				case SATS: lineOrder = 3; break;
				case EOH: lineOrder = 4;
				default: break;
				}
				break;
			case 3:	//SATS received. PRN can follow
				switch(labelId) {
				case VERSION:
				case SATS:
				case SYS: plog->warning(valueLabel(labelId, msgLabelRepeted)); break;
				case EOH: lineOrder = 4;
				default: break;
				}
			default:
				break;
			}
		}
	} while (maxErrors>0 && labelId!=LASTONE && lineOrder!=4);
	if (lineOrder != 4) plog->warning(valueLabel(EOH, msgNotFnd));
	return labelId;
}

/**readObsEpoch reads from a RINEX observation file one epoch (data and observables) and store them into the RinexData object.
 * Observable storage in the RinexData object is cleared before storing new data.
 * Stored epoch time and time tags (the same for epoch and observables) are set from epoch time read.
 *
 * @param input the already open print stream where RINEX epoch will be read
 * @return the status of the RINEX data read, which can can be:
 *		- (0)	EOF found. No epoch data stored.
 *		- (1)	Epoch observables and data are well formatted. They have been stored. 
 *		- (2)	Epoch event data are well formatted. They have been stored, and also special records. 
 *		- (3)	Error in observable data. Epoch data stored, but wrong observables stored as empty ones (0.0)
 *		- (4)	Error in epoch data. No epoch data stored.
 *		- (5)	Error in new site event: there is no MARKER NAME. Other event data and special recors stored.
 *		- (6)	Error reading special records following the epoch event. Other event data and special recors stored.
 *		- (7)	Error in external event: the record has no date.  Other event data and special recors stored.
 *		- (8)	Error in event flag number
 *		- (9)	Unknown input file version
 */
int RinexData::readObsEpoch(FILE* input) {
	epochObs.clear();
	switch(inFileVer) {
	case V210:
		return readV2ObsEpoch(input);
	case V304:
		return readV3ObsEpoch(input);
	default:
		return 9;
	}
}

/**readNavEpoch reads from the RINEX navigation file data and ephemeris for one setellite - epoch and store them into the RinexData object.
 * Ephemeris storage in the RinexData object is cleared before storing new data.
 *
 * @param input the already open print stream where RINEX epoch will be read
 * @return the status of the RINEX data read, which can can be:
 *		- (0)	EOF found. No epoch data stored.
 *		- (1)	Epoch navigation data are well formatted. They have been stored. They  belong to the current epoch
 *		- (2)	Epoch navigation data are well formatted. They have been stored. They  DO NOT belong to the current epoch
 *		- (3)	Error in satellite system or prn. No epoch data stored.
 *		- (4)	Error in epoch date or time format. No epoch data stored.
 *		- (5)	Error in epoch data. No epoch data stored.
 *		- (9)	Unknown input file version
 *		- (10)	Out of memory. No epoch data stored.
 */
int RinexData::readNavEpoch(FILE* input) {
///a macro to log the given error and return
#define LOG_ERR_AND_RETURN(ERROR_STR, ERROR_CODE) \
		{ \
			plog->warning(msgPrfx + ERROR_STR); \
			return ERROR_CODE; \
		}
///a macro to get data for broadcast orbit in LINE_I COL_J
#define GET_BO(LINE_I, COL_J) \
		if (sscanf(startPos1st, "%19lf", &bo[LINE_I][COL_J]) != 1) { \
			retCode = 5; \
			msgPrfx += msgErrBO + to_string((long long) LINE_I) + string("][") + to_string((long long) COL_J) + string("]."); \
		} \
		startPos1st += 19;

	char sysSat;
    char lineBuffer[100];
	int anInt, prnSat, nBroadcastOrbits, nEphemeris;
	double atow, attag;
	char *startPos1st;
	char *startPosBO;
	double bo[BO_MAXLINS][BO_MAXCOLS];
	int retCode;

	epochNav.clear();
	//read epoch 1st line and extract data and set specific line parameter
	if (readRinexRecord(lineBuffer, sizeof lineBuffer, input)) return 0;
	string msgPrfx =  msgEpoch + string(lineBuffer, 32) + msgBrak;
	int year = 0, month = 0, day = 0, hour = 0, minute = 0;
	double second = 0.0;
	switch (inFileVer) {
	case V210:
	    sysSat = sysToPrintId;
		if (sscanf(lineBuffer, "%2d", &prnSat) != 1) LOG_ERR_AND_RETURN(msgWrongSysPRN, 3)
		if (sscanf(lineBuffer+3, "%2d %2d %2d %2d %2d%5lf", &year, &month, &day, &hour, &minute, &second) != 6)
			LOG_ERR_AND_RETURN(msgWrongDate, 4)
		if (year >= 80) year += 1900;
		else year += 2000;
		startPos1st = lineBuffer + 22;	//start position of SV clock data in the 1st line
		startPosBO = lineBuffer + 3;	//start position of broadcat orbit data
		break;
	case V304:
		if (sscanf(lineBuffer, "%1c%2d", &sysSat, &prnSat) != 2) LOG_ERR_AND_RETURN(msgWrongSysPRN, 3)
		if (sscanf(lineBuffer+4, "%4d %2d %2d %2d %2d %2d", &year, &month, &day, &hour, &minute, &anInt) != 6)
			LOG_ERR_AND_RETURN(msgWrongDate, 4)
		second = (double) anInt;
		startPos1st = lineBuffer + 22;	//start position of SV clock data in the 1st line
		startPosBO = lineBuffer + 4;	//start position of broadcat orbit data
		break;
	default: LOG_ERR_AND_RETURN(msgWrongInFile, 9)
	}
	retCode = 1;
	for (int j = 1; j < BO_MAXCOLS; j++) { GET_BO(0, j) }
	switch (sysSat) {
	//set values for nBroadcastOrbits and nEphemeris as stated in RINEX 3.04 doc
		//set values for nBroadcastOrbits and nEphemeris as stated in RINEX 3.04 doc
		case 'G': nBroadcastOrbits = BO_MAXLINS_GPS; nEphemeris = 26; break;
		case 'E': nBroadcastOrbits = BO_MAXLINS_GAL; nEphemeris = 25; break;
		case 'C': nBroadcastOrbits = BO_MAXLINS_BDS; nEphemeris = 26; break;
		case 'S': nBroadcastOrbits = BO_MAXLINS_SBAS; nEphemeris = 12; break;
		case 'R': nBroadcastOrbits = BO_MAXLINS_GLO; nEphemeris = 12; break;
	    case 'J': nBroadcastOrbits = BO_MAXLINS_QZSS; nEphemeris = 26; break;
	    default: LOG_ERR_AND_RETURN(msgWrongSysPRN+string(1, sysSat), 2)
	}
	//read lines of broadcast orbit data in next lines
	for (int i = 1; (i < nBroadcastOrbits) && (nEphemeris > 0); i++) {
		if (readRinexRecord(lineBuffer, sizeof lineBuffer, input)) LOG_ERR_AND_RETURN(msgNoBO, 5);
		startPos1st = startPosBO;
		for (int j = 0; (j < BO_MAXCOLS) && (nEphemeris > 0); j++) {
			GET_BO(i, j)
			nEphemeris--;
		}
	}
	//set time and store data
	getWeekTowGPSdate(year, month, day, hour, minute, second, anInt, atow);
	attag = getInstantGNSStime(anInt, atow);
	if(epochNav.empty()) {	//set the time for the current epoch
		epochWeek = anInt;
		epochTOW = atow;
		epochTimeTag = attag;
	} else if(attag != epochTimeTag) {
		retCode = 2;
		msgPrfx += msgNewEp;
	}
	try {
		epochNav.push_back(SatNavData(attag, sysToPrintId, prnSat, bo));
		msgPrfx += msgStored;
	} catch (std::bad_alloc& ba) {
		LOG_ERR_AND_RETURN(msgNoMem + ba.what(), 10)
	}
	plog->fine(msgPrfx);
	return retCode;
#undef LOG_ERR_AND_RETURN
#undef GET_BO
}

//Class private methods
//=====================
/**setDefValues sets default values to optional RINEX data members, generation parameters, and
 * GPS navigation data constans (like scale factors and data).
 *
 * @param v the RINEX version to be generated
 * @param p a pointer to a Logger to be used to record logging messages
 */
void RinexData::setDefValues(RINEXversion v, Logger *p) {
	plog = p;
	//Header data
	//"RINEX VERSION / TYPE"
	version = v;
	inFileVer = VTBD;
	fileType = sysToPrintId = '?';
	//Epoch time data
	epochWeek = 0;
	epochTOW = epochTimeTag = epochClkOffset = 0.0;
	epochFlag = 0;
	//LEAP SECONDS
	//1st element in vector allways GPS, and default values set to 18 secs as per 2019
    leapSecs.push_back(LEAPsecs(18,0,0,0,'G'));
    //a table of system related descriptions (system identification, time description, system description, ...)
	sysDescript.push_back(SYSdescript('G', "GPS", ": GPS"));
	sysDescript.push_back(SYSdescript('M', "GPS", ": Mixed"));
	sysDescript.push_back(SYSdescript('R', "GLO", ": GLONASS"));
	sysDescript.push_back(SYSdescript('E', "GAL", ": Galileo"));
	sysDescript.push_back(SYSdescript('C', "BDT", ": Beidou"));
	sysDescript.push_back(SYSdescript('J', "QZS", ": QZSS"));
	sysDescript.push_back(SYSdescript('I', "IRN", ": IRNSS"));
	sysDescript.push_back(SYSdescript('S', "GPS", ": SBAS payload"));
    sysDescript.push_back(SYSdescript(' ', "GPS", ": GPS"));
	//fill vector with label definitions. Order is relevant.
	labelDef.push_back(LABELdata(VERSION,	"RINEX VERSION / TYPE",	VALL, OBSOBL + NAVOBL));
	labelDef.push_back(LABELdata(RUNBY,		"PGM / RUN BY / DATE",	VALL, OBSOBL + NAVOBL));
	labelDef.push_back(LABELdata(COMM,		"COMMENT",				VALL, OBSOPT + NAVOPT));
	labelDef.push_back(LABELdata(MRKNAME,	"MARKER NAME",			VALL, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(MRKNUMBER,	"MARKER NUMBER",		VALL, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(MRKTYPE,	"MARKER TYPE",			V304, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(AGENCY,	"OBSERVER / AGENCY",	VALL, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(RECEIVER,	"REC # / TYPE / VERS",	VALL, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(ANTTYPE,	"ANT # / TYPE",			VALL, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(APPXYZ,	"APPROX POSITION XYZ",	VALL, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(ANTHEN,	"ANTENNA: DELTA H/E/N",	VALL, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(ANTXYZ,	"ANTENNA: DELTA X/Y/Z",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(ANTPHC,	"ANTENNA: PHASECENTER",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(ANTBS,		"ANTENNA: B.SIGHT XYZ",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(ANTZDAZI,	"ANTENNA: ZERODIR AZI",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(ANTZDXYZ,	"ANTENNA: ZERODIR XYZ",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(COFM,		"CENTER OF MASS XYZ",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(WVLEN,		"WAVELENGTH FACT L1/2",	V210, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(TOBS,		"# / TYPES OF OBSERV",	V210, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(SYS,		"SYS / # / OBS TYPES",	V304, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(SIGU,		"SIGNAL STRENGTH UNIT",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(INT,		"INTERVAL",				VALL, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(TOFO,		"TIME OF FIRST OBS",	VALL, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(TOLO,		"TIME OF LAST OBS",		VALL, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(CLKOFFS,	"RCV CLOCK OFFS APPL",	VALL, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(DCBS,		"SYS / DCBS APPLIED",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(PCVS,		"SYS / PCVS APPLIED",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(SCALE,		"SYS / SCALE FACTOR",	V304, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(PHSH,		"SYS / PHASE SHIFTS",	V304, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(GLSLT,		"GLONASS SLOT / FRQ #",	V304, OBSOBL + NAVNAP));
    labelDef.push_back(LABELdata(GLPHS,     "GLONASS COD/PHS/BIS",  V304, OBSOBL + NAVNAP));
	labelDef.push_back(LABELdata(SATS,		"# OF SATELLITES",		VALL, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(PRNOBS,	"PRN / # OF OBS",		VALL, OBSOPT + NAVNAP));
	labelDef.push_back(LABELdata(IONA,		"ION ALPHA",			V210, OBSNAP + NAVOPT));
	labelDef.push_back(LABELdata(IONB,		"ION BETA",				V210, OBSNAP + NAVOPT));
	labelDef.push_back(LABELdata(IONC,		"IONOSPHERIC CORR",		V304, OBSNAP + NAVOPT));
	labelDef.push_back(LABELdata(DUTC,		"DELTA-UTC: A0,A1,T,W",	V210, OBSNAP + NAVOPT));
	labelDef.push_back(LABELdata(CORRT,		"CORR TO SYSTEM TIME",	V210, OBSNAP + NAVOPT));
	labelDef.push_back(LABELdata(GEOT,		"D-UTC A0,A1,T,W,S,U",	V210, OBSNAP + NAVOPT));
	labelDef.push_back(LABELdata(TIMC,		"TIME SYSTEM CORR",		V304, OBSNAP + NAVOPT));
	labelDef.push_back(LABELdata(LEAP,		"LEAP SECONDS",			VALL, OBSOPT + NAVOPT));
	labelDef.push_back(LABELdata(EOH,		"END OF HEADER",		VALL, OBSOBL + NAVOBL));

	labelDef.push_back(LABELdata(IONC_GAL,   "GAL ",	VALL, NAP));
	labelDef.push_back(LABELdata(IONC_GPSA,  "GPSA",	VALL, NAP));
	labelDef.push_back(LABELdata(IONC_GPSB,  "GPSB",	VALL, NAP));
	labelDef.push_back(LABELdata(IONC_QZSA,  "QZSA",	VALL, NAP));
	labelDef.push_back(LABELdata(IONC_QZSB,  "QZSB",	VALL, NAP));
	labelDef.push_back(LABELdata(IONC_BDSA,  "BDSA",	VALL, NAP));
	labelDef.push_back(LABELdata(IONC_BDSB,  "BDSB",	VALL, NAP));
	labelDef.push_back(LABELdata(IONC_IRNA,  "IRNA",	VALL, NAP));
	labelDef.push_back(LABELdata(IONC_IRNB,  "IRNB",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_GPUT,  "GPUT",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_GLUT,  "GLUT",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_GAUT,  "GAUT",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_BDUT,  "BDUT",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_BDGP,  "BDGP",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_QZUT,  "QZUT",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_IRUT,  "IRUT",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_SBUT,  "SBUT",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_GLGP,  "GLGP",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_GAGP,  "GAGP",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_QZGP,  "QZGP",	VALL, NAP));
	labelDef.push_back(LABELdata(TIMC_IRGP,  "IRGP",	VALL, NAP));

	labelDef.push_back(LABELdata(NOLABEL,	"No label detected",	VALL, NAP));
	labelDef.push_back(LABELdata(DONTMATCH,	"Incorrect label for this RINEX version", VALL, NAP));
	labelDef.push_back(LABELdata(LASTONE,	"Last item",	VALL, NAP));
	labelIdIdx = 0;
    for (numberV2ObsTypes = 0; !v3obsTypes[numberV2ObsTypes].empty(); numberV2ObsTypes++);
}

/**setFileDataType sets for the file type to be generated the values of the system to print (sysToPrint) and file type (fileType)
 * taking into accout the already defined version to print and the system or systems selected.
 * The value of sysToPrint is defined as TYPE in the RINEX VER/TYPE header record
 * The value of fileType is defined for the file name (t in .yyt in v2.10, second D in the DD data type in V3.04)
 * <p>Note that in RINEX V3.04 a navigation file can include ephemeris from several navigation systems,
 * but in V2.10 a navigation file can include data for only one system.
 * There are predefined comment lines, like the remark on printed V2.10 Galileo navigation files which are un-official versions,
 * that could be added or not to the header on request.
 *
 * @param ftype the file type ('O', 'N', ...) to be generated
 * @param setCOMMs when true, predefined comment lines are added to the header
 * @throws error message when file type or system to print cannot be set
 */
void RinexData::setFileDataType(char ftype, bool setCOMMs) {
    //identify the first system selected, and count the number of selected ones
    char firstSys = 0;
    for (vector<GNSSsystem>::iterator it = systems.begin(); it != systems.end(); it++) {
        if (it->selSystem)  {
            firstSys = it->system;
            break;
        }
    }
    int n = 0;
    for (vector<GNSSsystem>::iterator it = systems.begin(); it != systems.end(); it++) {
        if (it->selSystem) n++;
    }
    if (n == 0) throw msgNotSys;        //at least one system shall be selected
    //set default value for sysToPrintId
    if (n > 1) sysToPrintId = 'M';
    else sysToPrintId = firstSys;
    //set values according RINEX file version to be printed
    switch (ftype) {
        case 'O':
        case 'o':
            fileType = 'O';
            break;
        case 'N':
        case 'n':
            fileType = 'N';
            switch (version) {    //version to print
                case V210:
                    switch (firstSys) {
                        case 'G':    //GPS nav
                            sysToPrintId = fileType = 'N';
                            break;
                        case 'R':    //GLONASS nav
                            sysToPrintId = fileType = 'G';
                            break;
                        case 'E':    //Galileo nav. Un-official version
                            sysToPrintId = fileType = 'L';
                            if (setCOMMs) {
								setHdLnData(COMM, COMM, "This un-official version formats b.o. data as per V3.04");
								setHdLnData(COMM, COMM, "V2.10 does not define nav. data format for Galileo");
                            }
                            break;
                        case 'S':    //SBAS nav
                            sysToPrintId = fileType = 'B';
                            break;
                        default:
                            throw "Cannot generate navigation V2.10 file for system " + string(1, firstSys);
                            break;
                    }
                    break;
                case V304:
                    break;
                default:
                    throw msgVerTBD;
            }
            break;
        default:
            throw "Cannot generate files of type:" + string(1, ftype);
    }
}

/**fmtRINEXv2name format a standard RINEX V2.10 file name from the given designator, GPS week and TOW, and for the given type.
 *
 * @param designator the file name prefix with a 4-character station name designator
 * @param week the GPS week number whitout roll out (that is, increased by 1024 for current week numbers)
 * @param tow the seconds from the beginning of the week
 * @param ftype the file type ('O', 'N', ...)
 * @return the RINEX observation file name in the standard format (f.e.; PRFXdddamm.yyO)
 */
string RinexData::fmtRINEXv2name(string designator, int week, double tow) {
	char buffer[30];
	char yday2year[15];
	char dhour[5];
    formatGPStime(yday2year, sizeof yday2year, "%j_%M.%y", "", week, tow);
    formatGPStime(dhour, sizeof dhour, "%H", "", week, tow);
    yday2year[3] = 'a' + atoi(dhour);
    //format file name
    sprintf(buffer, "%4.4s%s%c",
            (designator + "----").c_str(),
            yday2year,
            fileType
    );
	return string(buffer);
}

/**fmtRINEXv3name format a standard RINEX V3.04 file name from the given designator code, GPS week and TOW, and for the given type.
 * For V3.1 the file name could include data from MARKER NUMBER, REC # / TYPE / VERS, TIME OF FIRST OBS and TIME OF LAST OBS,
 * INTERVAL, and SYS / # / OBS TYPES records in the header.
 *
 * @param designator the file name prefix with a 9-char station name designator (XXXXMRCCC), or a 4-char site/stations name
 * @param week the GPS week number whitout roll out (that is, increased by 1024 for current week numbers)
 * @param tow the seconds from the beginning of the week
 * @param country the 3-char ISO 3166-1 country code
 * @param ftype the file type ('O', 'N', ...)
 * @param country the 3-char ISO 3166-1 country code
 * @return the RINEX observation file name in the standard format (f.e.; PRFXdddamm.yyO)
 */
string RinexData::fmtRINEXv3name(string designator, int week, double tow, string country) {
	char buffer[50], startTime[20];
	//set value for field <SITE/STATION/MONUMENT/RECEIVER/COUNTRY/> (XXXXMRCCC) if not given
	if (designator.length() != 9) {
		strcpy(buffer, ((designator + "------").substr(0, 6) + (country + "---").substr(0, 3)).c_str());	//set  buffer to XXXX--CCC
		if (getLabelFlag(MRKNUMBER)) buffer[4] = getFirstDigit(markerNumber, '-');
		if (getLabelFlag(RECEIVER)) buffer[5] = getFirstDigit(rxNumber, '-');
		designator = string(buffer);
	}
	//set value for field <START TIME>
	formatGPStime(startTime, sizeof startTime, "%Y%j%H%M", "", week, tow);
	//set value for field <FILE PERIOD> (period value and unit from TOFO, TOLO)
	int period = 0;
	char periodUnit = 'U';
	double periodStart, periodEnd;
	if (getLabelFlag(TOFO) && getLabelFlag(TOLO)) {
		periodStart = getInstantGNSStime (firstObsWeek, firstObsTOW);
		periodEnd = getInstantGNSStime (lastObsWeek, lastObsTOW);
		if (periodEnd > periodStart) period = (int) ((periodEnd - periodStart) / 60);
	}
	if (period >= 365*24*60) {
		period /= 365*24*60;
		periodUnit = 'Y';
	}
	else if (period >= 24*60) {
		period /= 24*60;
		periodUnit = 'D';
	}
	else if (period >= 60) {
		period /= 60;
		periodUnit = 'H';
	}
	else if (period > 0) {
		periodUnit = 'M';
	}
	//set value for field <DATA FREQ> (value and unit from INTERVAL)
	int frequency = 0;
	char frequencyUnit = 'U';
	if (getLabelFlag(INT)) {
		if ((obsInterval < 1) && (obsInterval > 0)) {
			frequency = (int) (1.0 / obsInterval);
			frequencyUnit = 'Z';
		} else if (obsInterval < 60) {
			frequency = (int) obsInterval;
			frequencyUnit = 'S';
		} else if (obsInterval < 60*60) {
			frequency = (int) (obsInterval / 60);
			frequencyUnit = 'M';
		} else if (obsInterval < 60*60*24) {
			frequency = (int) (obsInterval / 60 / 60);
			frequencyUnit = 'H';
		} else {
			frequency = (int) (obsInterval / 60 / 60 / 24);
			frequencyUnit = 'D';
		}
	}
	//compose the file name
	sprintf(buffer, "%9.9s_R_%s_%02d%c_%02d%c_%c%c.rnx",
			designator.c_str(),
			startTime,
			period,
			periodUnit,
			frequency,
			frequencyUnit,
			sysToPrintId,
			fileType);
	return string(buffer);
}

/**setLabelFlag sets the hasData flag of the given label to the given value (by default to true)
 * As the record for this label is assumed was modified, lastRecordSet is modified accordingly.
 * 
 * @param label is the record label identifier
 * @param flagVal is the value to set (by default true)
 */
void RinexData::setLabelFlag(RINEXlabel label, bool flagVal) {
	lastRecordSet = labelDef.end();
	for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it)
		if(it->labelID == label) {
			it->hasData = flagVal;
			lastRecordSet = it;
			return;
		}
}

/**sgetLabelFlag gets the hasData flag value of the given label
 * 
 * @param label is the record label identifier
 * @return the value stored in the hasData flag for this labelId, or false if the label does not exist
 */
bool RinexData::getLabelFlag(RINEXlabel label) {
	for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it)
		if(it->labelID == label) return it->hasData;
	return false;
}

/**checkLabel checks if the RINEX line passed ends with a correct RINEX header label for the input file version
 * Note that RINEX header lines contain label in columns 61 to 80 (index 60 to 79).
 *
 * @param line is a null terminated char sequence containing the RINEX line to analyze
 * @return the RINEX label identification, NOLABEL has not a valid RINEX lable, or DONTMATCH if the label is not valid for the stated RINEX version 
 */
RinexData::RINEXlabel RinexData::checkLabel(char *line) {
	//label shall be in columns 61 to 80 (index 60 to 79)
	if (strlen(line) < 61) return NOLABEL;
	char *label = &line[60];
	for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it)
		if (strncmp(label, it->labelVal, strlen(it->labelVal)) == 0) {
			if ((it->ver == VALL) || (it->ver == inFileVer)) return it->labelID;
			else return DONTMATCH;
		}
	return NOLABEL;
}

/**findLabelId finds for the label passed the corresponding identifier
 *
 * @param line is a null terminated char sequence containing the RINEX label to identify
 * @return the RINEX label identification, NOLABEL has not a valid RINEX lable,
 */
RinexData::RINEXlabel RinexData::findLabelId(char *label) {
	for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it)
		if (strncmp(label, it->labelVal, strlen(it->labelVal)) == 0) return it->labelID;
	return NOLABEL;
}

/**valueLabel gives the sting value for the RINEXlabel passed
 * 
 * @param labelId is the label identifier
 * @return a string with the label value
 */
string RinexData::valueLabel(RINEXlabel labelId, string toAppend) {
    const string msgErrUnkLabel("Unknown label identifier");
	for (vector<LABELdata>::iterator it = labelDef.begin() ; it != labelDef.end(); ++it)
		if(it->labelID == labelId) {
			if (toAppend.empty()) return string(it->labelVal);
			else return string(it->labelVal) + msgColon + toAppend;
		}
	return msgErrUnkLabel;
}

/**readV2ObsEpoch reads from the RINEX version 2.1 observation file data lines of an epoch.
 * Note that observation records in RINEX V2.1 have a limited size of 80 characters.
 * For this reason, continuation lines could exits for the 1st epoch line having more than 12 satellites
 * and for the measurement lines having more than 5 measurements per satellite.
 *
 * @param input the already open print stream where RINEX epoch will be read
 * @return the status of the RINEX data read, which can can be:
 *		- (0)	EOF found. No epoch data stored.
 *		- (1)	Epoch observable data are well formatted. They have been stored. 
 *		- (2)	Epoch event data are well formatted. They have been stored, and also special records. 
 *		- (3)	Error in observables. Epoch data stored, but wrong obsservables stored as empty ones
 *		- (4)	Error in epoch data. No epoch data stored.
 *		- (5)	Error in new site event: there is no MARKER NAME. Other event data and special recors stored.
 *		- (6)	Error reading special records following the epoch event. Other event data and special recors stored.
 *		- (7)	Error in external event: the record has no date.  Other event data and special recors stored.
 *		- (8)	Error in event flag number
 */
int RinexData::readV2ObsEpoch(FILE* input) {
    const string msgWrongNsats(" Wrong number of sats (>64).");
    const string msgEOFinCont(" EOF in epoch cont. line.");
    const string msgUnexpEOF2("Unexpected EOF in observation continuation record");
	char lineBuffer[100];
	int posPRN, nObs, posObs;
	unsigned int sysInEpoch[64];
	int prnInEpoch[64];
	double valObs;
	int lliObs, strgObs;
	int i, j, k;

	//read epoch 1st line and extract data
	if (readRinexRecord(lineBuffer, sizeof lineBuffer, input)) return 0;
	string msgPrfx =  msgEpoch + string(lineBuffer, 32) + msgBrak;
	bool badEpoch = false;
	if ((epochFlag = (int) (lineBuffer[28] - '0')) < 0) {
		badEpoch = true;
		msgPrfx  += msgNoFlag;
		epochFlag = 999;	//a nonexisting flag
	}
	if (isBlank(lineBuffer + 29, 3)) {
		badEpoch = true;
		msgPrfx += msgSatOrSp;
		nSatsEpoch = 0;
	} else nSatsEpoch = stoi(string(lineBuffer+29, 3));
	int year = 0, month = 0, day = 0, hour = 0, minute = 0;
	double second = 0.0;
	bool wrongDate = sscanf(lineBuffer, " %2d %2d %2d %2d %2d%11lf", &year, &month, &day, &hour, &minute, &second) != 6 ;
	if (year >= 80) year += 1900;
	else year += 2000;
	if (!wrongDate) {	//translate date read to week + tow
		getWeekTowGPSdate (year, month, day, hour, minute, second, epochWeek, epochTOW);
		epochTimeTag = getInstantGNSStime(epochWeek, epochTOW);
	}
	switch (epochFlag) {
	case 0:
	case 1:
	case 6:
		if (wrongDate) {
			badEpoch = true;
			msgPrfx += msgWrongDate;
		}
		if (nSatsEpoch > 64) {
			badEpoch = true;
			msgPrfx += msgWrongNsats;
		}
		if (isBlank(lineBuffer + 68, 12)) epochClkOffset = 0.0;
		else epochClkOffset = stod(string(lineBuffer + 68, 12));
		//get satellites from epoch 1st line and eventual continuation lines (max 12 sat id in each one)
		for (i=0; i<nSatsEpoch; i+=12) {
			for(j=0, posPRN = 32; j<12 && i+j<nSatsEpoch; j++, posPRN += 3) {
				try {
					sysInEpoch[i+j] = getSysIndex(lineBuffer[posPRN]);
				}  catch (string error) {
					badEpoch = true;
					msgPrfx += error;
				}
				if (sscanf(lineBuffer+posPRN+1, "%2d", &prnInEpoch[i+j]) !=1 ) {
					badEpoch = true;
					msgPrfx += msgWrongPRN;
				}
			}
			if (i+j < nSatsEpoch) {	//read continuation line
				if (readRinexRecord(lineBuffer, sizeof lineBuffer, input)) {
					msgPrfx += msgEOFinCont;
				}
			}
		}
		if (badEpoch) {
			//if any error in epoch line record, try to skip observation data lines
			for (i=0; i<nSatsEpoch; i++) readRinexRecord(lineBuffer, sizeof lineBuffer, input);
			plog->warning(msgPrfx);
			return 4;
		}
		//read the observation records for each satellite in the epoch
		for (i=0; i<nSatsEpoch; i++) {
			if (readRinexRecord(lineBuffer, sizeof lineBuffer, input)) {
				plog->warning(msgPrfx + msgUnexpObsEOF);
				return 3;
			}
			nObs = systems[sysInEpoch[i]].obsTypes.size();
			//for each observable type in this satellite extracts its data from the record
			//each record can have data for 5 observable types (or less). Continuation records are used when needed 
			for (j=0; j<nObs; j+=5) {
				for (k=0, posObs = 0; k<5 && j+k<nObs; k++, posObs += 16) {
					if (isBlank(lineBuffer + posObs, 14)) {	//empty observable
						epochObs.push_back(SatObsData(epochTimeTag, sysInEpoch[i], prnInEpoch[i], j+k, 0.0, 0, 0));
					} else {
						valObs = stod(string(lineBuffer+posObs, 14));
						if (lineBuffer[posObs+14] == ' ') lliObs = 0;
						else lliObs = (int) (lineBuffer[posObs+14] - '0');
						if (lineBuffer[posObs+15] == ' ') strgObs = 0;
						else strgObs = (int) (lineBuffer[posObs+15] - '0');
						epochObs.push_back(SatObsData(epochTimeTag, sysInEpoch[i], prnInEpoch[i], j+k, valObs, lliObs, strgObs ));
					}
				}
				if (j+k < nObs) {
					if (readRinexRecord(lineBuffer, sizeof lineBuffer, input)) {
						plog->warning(msgPrfx + msgUnexpEOF2);
						return 3;
					}
				}
			}
		}
		plog->fine(msgPrfx);
		return 1;
	case 2:
	case 3:
	case 4:
	case 5:
		plog->fine(msgPrfx);
		return readObsEpochEvent(input, wrongDate);
	default:
		plog->warning(msgPrfx + msgWrongFlag);
		return 8;
	}
}

/**readV3ObsEpoch reads from the RINEX version 3.0 observation file an epoch data
 * Note that observation records in RINEX V3.04 do not have a size limit.
 *
 * @param input the already open print stream where RINEX epoch will be read
 * @return status of observable or event data read according to:
 *		- (0)	EOF found, no epoch or event data available
 *		- (1)	Epoch flag was 0. Observation data read and stored
 *		- (2)	Epoch flag was 1. Observation data read and stored
 *		- (3)	Epoch flag was 2: Kinematic data flag set to true
 *		- (4)	Epoch flag was 3: Kinematic data flag set to false and data in header lines following read
 *		- (5)	Epoch flag was 4: Data in header lines following read
 *		- (6)	Epoch flag was 5: Significant epoch flag set to true
 *		- (7)	Epoch flag is 6: Cycle slip data read and stored (same format as observables)
 * @throws error string with message describing any error detected in data format
 */
int RinexData::readV3ObsEpoch(FILE* input) {
    const string msgWrongStart(" Wrong start of epoch. Line skip");
	char lineBuffer[1300]; //enough big to allocate 3 + 2 + 19 x 4 measurements x 16 chars= 1221
	int nObs, posObs;
	unsigned int sysIdx;
	int prnSat;
	double valObs;
	int lliObs, strgObs;
	int i, j;
	string msgPrfx, aStr;
	//read epoch 1st line and extract data
	for (;;) {	//synchronize start of epoch
		if (readRinexRecord(lineBuffer, sizeof lineBuffer, input)) return 0;
		msgPrfx =  msgEpoch + string(lineBuffer, 35) + msgBrak;
		if (lineBuffer[0] == '>') break;
		plog->warning(msgPrfx + msgWrongStart);
	}
	bool badEpoch = false;
	if ((epochFlag = (int) (lineBuffer[31] - '0')) < 0) {
		badEpoch = true;
		msgPrfx  += msgNoFlag;
		epochFlag = 999;	//a nonexisting flag
	}
	if (isBlank(lineBuffer + 32, 3)) {
		badEpoch = true;
		msgPrfx += msgSatOrSp;
		nSatsEpoch = 0;
	} else nSatsEpoch = stoi(string(lineBuffer + 32, 3));
	int year = 0, month = 0, day = 0, hour = 0, minute = 0;
	double second = 0.0;
	bool wrongDate = sscanf(lineBuffer+2, "%4d %2d %2d %2d %2d%11lf", &year, &month, &day, &hour, &minute, &second) != 6 ;
	if (!wrongDate) {	//translate date read to week + tow
		getWeekTowGPSdate (year, month, day, hour, minute, second, epochWeek, epochTOW);
		epochTimeTag = getInstantGNSStime(epochWeek, epochTOW);
	}
	switch (epochFlag) {
	case 0:
	case 1:
	case 6:
		if (wrongDate) {
			badEpoch = true;
			msgPrfx += msgWrongDate;
		}
		if (badEpoch) {
			plog->warning(msgPrfx);
			return 4;
		}
		if (isBlank(lineBuffer + 41, 15)) epochClkOffset = 0.0;
		else epochClkOffset = stod(string(lineBuffer + 41, 15));
		//get the observation record for each satellite and extract data
		for (i = 0; i < nSatsEpoch; i++) {
			if (readRinexRecord(lineBuffer, sizeof lineBuffer, input)) {
				plog->warning(msgPrfx + msgUnexpObsEOF);
				return 3;
			}
			try {
				sysIdx = getSysIndex(lineBuffer[0]);
				if (sscanf(lineBuffer+1, "%2d", &prnSat) == 1) {
					//for each observable type in the system of this satellite
					nObs = systems[sysIdx].obsTypes.size();
					for (j = 0, posObs = 3; j < nObs; j++, posObs += 16) {
						if (isBlank(lineBuffer + posObs, 14)) {
							//empty observable: values are considered 0
							epochObs.push_back(SatObsData(epochTimeTag, sysIdx, prnSat, j, 0.0, 0, 0));
						} else {
							valObs = stod(string(lineBuffer+posObs, 14));
							if (lineBuffer[posObs+14] == ' ') lliObs = 0;
							else lliObs = stoi(string(lineBuffer+posObs+14, 1));
							if (lineBuffer[posObs+15] == ' ') strgObs = 0;
							else strgObs = stoi(string(lineBuffer+posObs+15, 1));
							epochObs.push_back(SatObsData(epochTimeTag, sysIdx, prnSat, j,  valObs, lliObs, strgObs));
						}
					}
				} else {
					badEpoch = true;
					msgPrfx += msgWrongPRN;
				}
			}  catch (string error) {
				badEpoch = true;
				msgPrfx += error;
			}
		}
		if (badEpoch) {
			plog->warning(msgPrfx);
			return 3;
		}
		plog->fine(msgPrfx);
		return 1;
	case 2:
	case 3:
	case 4:
	case 5:
		plog->fine(msgPrfx);
		return readObsEpochEvent(input, wrongDate);
	default:
		plog->warning(msgPrfx + msgWrongFlag);
		return 8;
	}
}

/**readObsEpochEvent reads from the RINEX observation file event records
 * 
 * @param input the already open print stream where RINEX epoch will be read
 * @param wrongDate a flag indicating if the current epoch date and time are correct or not
 * @return status of observable or event data read according to:
 *		- (2)	Epoch event data are well formatted. They have been stored, and also special records. 
 *		- (5)	Error in new site event: there is no MARKER NAME. Other event data and special recors stored.
 *		- (6)	Error reading special records following the epoch event. Other event data and special recors stored.
 *		- (7)	Error in external event: the record has no date.  Other event data and special recors stored.
 *		- (8)	Error in event flag number
 */
int RinexData::readObsEpochEvent(FILE* input, bool wrongDate) {
	RINEXlabel labelId;
	bool mrknReceived = false;
	int retValue = 2;
	switch (epochFlag) {
	case 2:	//start moving antenna event
		for (int i=0; i<nSatsEpoch; i++) {
			labelId = readHdLineData(input);
			switch (labelId) {
			case NOLABEL:
			case LASTONE:
				plog->warning(msgKinemEvent);
				retValue = 6;
			default: break;
			}
		}
		break;
	case 3:	//new site occupation  event (at least MARKER NAME record follows)
		retValue = 5;	//default
		for (int i=0; i<nSatsEpoch; i++) {
			labelId = readHdLineData (input);
			switch (labelId) {
			case MRKNAME:
				mrknReceived = true;
				retValue = 2;
				break;
			case NOLABEL:
			case LASTONE:
				plog->warning(msgOccuEvent);
				retValue = 6;
			default: break;
			}
		}
		if (!mrknReceived) plog->warning(msgOccuEventNoMark);
		break;
	case 4:	//header information follows
		for (int i=0; i<nSatsEpoch; i++) {
			labelId = readHdLineData (input);
			switch (labelId) {
			case NOLABEL:
			case LASTONE:
				plog->warning(msgHdEvent);
				retValue = 6;
			default: break;
			}
		}
		break;
	case 5:	//external event (epoch is significant)
		if (wrongDate) {
			plog->warning(msgExtEvent);
			return 7;
		}
		break;
	default:
		retValue = 8;
	}
	return retValue;
}

/**printHdLineData prints a header line data into the output file
 * 
 * @param out the already open print stream where RINEX header line will be printed
 * @param labelId is the label identifier for the line to be printed
 */
void RinexData::printHdLineData(FILE* out, vector<LABELdata>::iterator lbIter) {
	///a macro to print a SYS / type record
	#define PRINT_SYSREC(VECTOR, ITEMS_PER_LINE, PRNTPFX_1ST, PRNTPFX_CON, PRNT_ITEM, PRNT_EMPTYITEM) \
		/*in a VECTOR, print ITEMS_PER_LINE VECTOR elements per line (in a 1st line + continuation lines if needed)*/ \
 		if ((k = VECTOR.size()) != 0) { \
 			for (j = 0; j < k; j++) { \
				if ((j % ITEMS_PER_LINE) == 0) {	\
					if (j == 0) n = PRNTPFX_1ST;  /*print the 1st line prefix*/ \
					else {	/*finish current line and print the continuation line prefix*/ \
						fprintf(out, "%s%-20s\n", string(60-n,' ').c_str(), valueLabel(labelId).c_str()); \
						n = PRNTPFX_CON; \
					} \
				} \
 				n += PRNT_ITEM; \
 			} \
			while ((j++ % ITEMS_PER_LINE) != 0) n += PRNT_EMPTYITEM; /*print empty data to complete line*/\
			fprintf(out, "%s%-20s\n", string(60-n,' ').c_str(), valueLabel(labelId).c_str());  /*finish line printing line label*/\
 		}

    const string msgUnkSys("Unknown satellite system=");
	unsigned int i, j, k, n;
	char timeBuffer[80], cnsId;
	string aStr;
	vector<string> aVectorStr;
	vector<bool> aVectorBool;
	int year, month, day;

    double instant;

	RINEXlabel labelId = lbIter->labelID;
	switch (labelId) {
    case VERSION:    //"RINEX VERSION / TYPE"
        if (version == V210) {  //print VERSION params as per V210
            if (fileType == 'O') {
                fprintf(out, "%9.2f%11c%1c%-19.19s%1c%-19.19s", 2.10, ' ', fileType, fileTypeSfx.c_str(), sysToPrintId, systemIdSfx.c_str());
            } else {
                fprintf(out, "%9.2f%11c%1c%-19.19s%1c%-19.19s", 2.10, ' ', fileType, fileTypeSfx.c_str(), ' ', " ");
            }
        } else {    //by default V304
            fprintf(out, "%9.2f%11c%1c%-19.19s%1c%-19.19s", 3.04, ' ', fileType, fileTypeSfx.c_str(), sysToPrintId, systemIdSfx.c_str());
        }
        break;
    case RUNBY:        //"PGM / RUN BY / DATE"
        if (date.length() == 0) {
            //get current UTC time and format it
            formatUTCtime(timeBuffer, sizeof timeBuffer, "%Y%m%d %H%M%S ");
            fprintf(out, "%-20.20s%-20.20s%s%3s ", pgm.c_str(), runby.c_str(), timeBuffer, "UTC");
        } else {
            fprintf(out, "%-20.20s%-20.20s%-20.20s", pgm.c_str(), runby.c_str(), date.c_str());
        }
        break;
    case COMM:        //"COMMENT"
        fprintf(out, "%-60.60s", (lbIter->comment).c_str());
        break;
    case MRKNAME:    //"MARKER NAME"
        fprintf(out, "%-60.60s", markerName.c_str());
    	break;
    case MRKNUMBER:    //"MARKER NUMBER"
        fprintf(out, "%-60.60s", markerNumber.c_str());
        break;
    case MRKTYPE:    //"MARKER TYPE"
        fprintf(out, "%-20.20s%40c", markerType.c_str(), ' ');
        break;
    case AGENCY:    //"OBSERVER / AGENCY"
    	fprintf(out, "%-20.20s%-40.40s", observer.c_str(), agency.c_str());
        break;
    case RECEIVER:    //"REC # / TYPE / VERS
    	fprintf(out, "%-20.20s%-20.20s%-20.20s", rxNumber.c_str(), rxType.c_str(), rxVersion.c_str());
        break;
    case ANTTYPE:    //"ANT # / TYPE"
        fprintf(out, "%-20.20s%-20.20s%20c", antNumber.c_str(), antType.c_str(), ' ');
        break;
    case APPXYZ:    //"APPROX POSITION XYZ"
        fprintf(out, "%14.4lf%14.4lf%14.4lf%18c", aproxX, aproxY, aproxZ, ' ');
    	break;
    case ANTHEN:        //"ANTENNA: DELTA H/E/N"
        fprintf(out, "%14.4lf%14.4lf%14.4lf%18c", antHigh, eccEast, eccNorth, ' ');
        break;
    case ANTXYZ:        //"ANTENNA: DELTA X/Y/Z"	V300
        fprintf(out, "%14.4lf%14.4lf%14.4lf%18c", antX, antY, antX, ' ');
        break;
    case ANTPHC:        //"ANTENNA: PHASECENTE"		V300
        fprintf(out, "%c %-3.3s%9.4lf%14.4lf%14.4lf%18c", antPhSys, antPhCode.c_str(), antPhNoX, antPhEoY, antPhUoZ, ' ');
        break;
    case ANTBS:            //"ANTENNA: B.SIGHT XYZ"	V300
        fprintf(out, "%14.4lf%14.4lf%14.4lf%18c", antBoreX, antBoreY, antBoreX, ' ');
        break;
    case ANTZDAZI:        //"ANTENNA: ZERODIR AZI"	V300
        fprintf(out, "%14.4lf%46c", antZdAzi, ' ');
        break;
    case ANTZDXYZ:        //"ANTENNA: ZERODIR XYZ"	V300
        fprintf(out, "%14.4lf%14.4lf%14.4lf%18c", antZdX, antZdY, antZdX, ' ');
        break;
    case COFM :            //"CENTER OF MASS XYZ"		V300
        fprintf(out, "%14.4lf%14.4lf%14.4lf%18c", centerX, centerY, centerX, ' ');
        break;
    case WVLEN:            //"WAVELENGTH FACT L1/2"	V210
        for (vector<WVLNfactor>::iterator it = wvlenFactor.begin(); it != wvlenFactor.end(); it++) {
            n = it->satNums.size();
            fprintf(out, "%6d%6d%6d", it->wvlenFactorL1, it->wvlenFactorL2, n);
            for (i = 0; i < 7; i++)
                if (i < n) fprintf(out, "%3c%3s", ' ', it->satNums[i].c_str());
                else fprintf(out, "%6c", ' ');
            fprintf(out, "%-20.20s\n", valueLabel(labelId).c_str());
        }
        return;
    case TOBS:        //"# / TYPES OF OBSERV"		V210
        if (systems.empty()) return;
		//Note that only V210 obsTypes are taken into account
        //copy into aVectorStr the V210 obsTypes identifiers to be printed
        //note that all systems have the same observables to print
        aVectorStr.clear();
        for (i = 0; i < numberV2ObsTypes; i++)
            if (systems[0].obsTypes[i].prt) aVectorStr.push_back(v2obsTypes[i]);
        PRINT_SYSREC(aVectorStr,
    	        9,
        	    fprintf(out, "%6u", k),
	            fprintf(out, "%6c", ' '),
    	        fprintf(out, "%4c%2.2s", ' ', aVectorStr[j].c_str()),
        	    fprintf(out, "%6c", ' ')
		)
		return;
	case SYS :		//"SYS / # / OBS TYPES"		V300
		//for each system, print 13 observable types per line (a 1st line + continuation lines if needed)
        //to do it, copy into aVectorStr the obsTypes identifications to be printed
        for (vector<GNSSsystem>::iterator itsys = systems.begin(); itsys != systems.end(); itsys++) {
			aVectorStr.clear();
			for (vector<OBSmeta>::iterator itobs = itsys->obsTypes.begin(); itobs != itsys->obsTypes.end(); itobs++) {
			    itobs->prt = itobs->sel;
			    if (itobs->prt) aVectorStr.push_back(itobs->id);
			}
		    PRINT_SYSREC(aVectorStr,
				13,
				fprintf(out, "%1c  %3u", itsys->system, k),
				fprintf(out, "%6c", ' '),
				fprintf(out, " %3s", aVectorStr[j].c_str()),
				fprintf(out, "%4c", ' ')
            )
 		}
		return;
	case SIGU :		//"SIGNAL STRENGTH UNIT"
		fprintf(out, "%-20.20s%40c", signalUnit.c_str(), ' ');
		break;
	case INT :		//"INTERVAL"
	 	fprintf(out, "%10.3lf%50c", obsInterval, ' ');
		break;
	case TOFO :		//"TIME OF FIRST OBS"
		formatGPStime (timeBuffer, sizeof timeBuffer, "  %Y    %m    %d    %H    %M  ", "%11.7lf", firstObsWeek, firstObsTOW);
		// fprintf(out, "%s%5c%-3.3s%9c", timeBuffer, ' ', obsTimeSys.c_str(), ' ');
		fprintf(out, "%s%5c%-3.3s%9c", timeBuffer, ' ', getTimeDes(obsTimeSys).c_str(), ' ');
		break;
	case TOLO :		//"TIME OF LAST OBS"
		formatGPStime (timeBuffer, sizeof timeBuffer, "  %Y    %m    %d    %H    %M  ", "%11.7lf", lastObsWeek, lastObsTOW);
		// fprintf(out, "%s%5c%-3.3s%9c", timeBuffer, ' ', obsTimeSys.c_str(), ' ');
		fprintf(out, "%s%5c%-3.3s%9c", timeBuffer, ' ', getTimeDes(obsTimeSys).c_str(), ' ');
		break;
	case CLKOFFS :	//"RCV CLOCK OFFS APPL"
	 	fprintf(out, "%6d%54c", rcvClkOffs, ' ');
		break;
	case DCBS :		//"SYS / DCBS APPLIED"
		for (vector<DCBSPCVSapp>::iterator it = dcbsApp.begin(); it != dcbsApp.end(); ++it)
			if (systems[it->sysIndex].selSystem) {
				fprintf(out, "%c %-17.17s %-40.40s", systems[it->sysIndex].system, it->corrProg.c_str(), it->corrSource.c_str());
				fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
			}
		return;
	case PCVS :		//"SYS / PCVS APPLIED"
		for (vector<DCBSPCVSapp>::iterator it = pcvsApp.begin(); it != pcvsApp.end(); ++it)
			if (systems[it->sysIndex].selSystem) {
				fprintf(out, "%c %-17.17s %-40.40s", systems[it->sysIndex].system, it->corrProg.c_str(), it->corrSource.c_str());
				fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
			}
		return;
	case SCALE :	//"SYS / SCALE FACTOR"
		//for each record, print 12 observable types per line (a 1st line + continuation lines if needed)
        for (vector <OSCALEfact>::iterator it = obsScaleFact.begin(); it != obsScaleFact.end(); it++)
            if (systems[it->sysIndex].selSystem) {
                PRINT_SYSREC(it->obsType,
                             12,
                             fprintf(out, "%c %4d  %2u",  systems[it->sysIndex].system,  it->factor, k),
                             fprintf(out, "%10c", ' '),
                             fprintf(out, " %-3.3s", it->obsType[j].c_str()),
                             fprintf(out, "%4c", ' ') )
            }
		return;
	case PHSH :	//"SYS / PHASE SHIFTS"
		//for each record, print 10 satellites per line (a 1st line + continuation lines if needed)
        for (vector <PHSHcorr>::iterator it = phshCorrection.begin(); it != phshCorrection.end(); it++)
            if (systems[it->sysIndex].selSystem) {
                if (it->obsCode.empty() && (it->correction == 0.0)) {
                    fprintf(out, "%c %s%-20s\n", systems[it->sysIndex].system, string(58,' ').c_str(), valueLabel(labelId).c_str());
                } else {
                    PRINT_SYSREC(it->obsSats,
                                 10,
                                 fprintf(out, "%c %-3.3s %8.5lf  %2u", systems[it->sysIndex].system, it->obsCode.c_str(), it->correction, k),
                                 fprintf(out, "%18c", ' '),
                                 fprintf(out, " %-3.3s", it->obsSats[j].c_str()),
                                 fprintf(out, "%4c", ' ')
                    )
                }
            }
        return;
	case GLSLT:	//*GLONASS SLOT / FRQ #
		PRINT_SYSREC(gloSltFrq,
					 8,
					 fprintf(out, "%3d ", (int) gloSltFrq.size()),
					 fprintf(out, "%4c", ' '),
					 fprintf(out, "R%-2.2d %2d ", gloSltFrq[j].slot, gloSltFrq[j].frqNum),
					 fprintf(out, "%7c", ' ') )
		return;
	case GLPHS:	//"GLONASS COD/PHS/BIS"
		PRINT_SYSREC(gloPhsBias,
					 4,
					 fprintf(out, ""),
					 fprintf(out, ""),
					 fprintf(out, " %-3.3s %8.3lf", gloPhsBias[j].obsCode.c_str(), gloPhsBias[j].obsCodePhaseBias),
					 fprintf(out, "%13c", ' ') )
		return;
	case LEAP :		//"LEAP SECONDS"
		if (version == V304) {
			for (vector<LEAPsecs>::iterator it = leapSecs.begin(); it != leapSecs.end(); it++) {
				fprintf(out, "%6d%6d%6d%6d", it->secs, it->deltaLSF, it->weekLSF, it->dayLSF);
				if (leapSysId == 'C') fprintf(out, "BDS%33c", ' ');
				else fprintf(out, "%36c", ' ');
                fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
			}
			return;
		}
		fprintf(out, "%6d%54c", leapSecs[0].secs, ' ');
		break;
	case SATS :		//"# OF SATELLITES"
	 	fprintf(out, "%6d%54c", numOfSat, ' ');
		break;
	case PRNOBS :	//"PRN / # OF OBS"
		//for each record, print 9 observable types per line (a 1st line + continuation lines if needed)
        for (vector<PRNobsnum>::iterator it = prnObsNum.begin(); it != prnObsNum.end(); it++) {
            PRINT_SYSREC(it->obsNum,
                         9,
                         fprintf(out, "   %c%-2.2d", it->sysPrn, it->satPrn),
                         fprintf(out, "%6c", ' '),
                         fprintf(out, "%6d", it->obsNum[j]),
                         fprintf(out, "%6c", ' ')
            )
        }
		return;
	case IONA :		//"ION ALPHA"			(in GPS NAV version V210)
		for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); it++) {
			if (it->corrType == IONC_GPSA) {
				fprintf(out, "%2c", ' ');
				for (i = 0; i < 4; i++) {
					fprintf(out, "%12.4lE", it->corrValues[i]);
				}
				fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
			}
		}
		return;
	case IONB :		//"ION BETA"				(in GPS NAV version V210)
		for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); it++) {
			if (it->corrType == IONC_GPSB) {
				fprintf(out, "%2c", ' ');
				for (i = 0; i < 4; i++) {
					fprintf(out, "%12.4lE", it->corrValues[i]);
				}
				fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
			}
		}
		return;
	case IONC :		//"IONOSPHERIC CORR"		GNSS nav V304
		for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); it++) {
		    if (isIonoCorrection(it->corrType)) {
				fprintf(out, "%-4.4s ", valueLabel(it->corrType).c_str());
				for (i = 0; i < 4; i++) {
					fprintf(out, "%12.4lE", it->corrValues[i]);
				}
				fprintf(out, " %c %-2.2d  ", ((((int) it->corrValues[4]) / 60 / 60) % 24) + 'A', (int) it->corrValues[5]);
				fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
		    }
		}
		return;
	case DUTC :		//"DELTA-UTC: A0,A1,T,W"	(in GPS NAV version V210)
		for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); ++it) {
			if (it->corrType == TIMC_GPUT) {
				fprintf(out, "%3c", ' ');
				for (i = 0; i < 2; i++) fprintf(out, "%19.12lE", it->corrValues[i]);
				fprintf(out, "%9d%9d", (int) it->corrValues[2], (int) it->corrValues[3]);
				fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
			}
		}
		return;
	case CORRT:     //"CORR TO SYSTEM TIME"  (in GLONASS NAV version v210)
        for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); ++it) {
            if (it->corrType == TIMC_GLUT) {
                formatGPStime(timeBuffer, sizeof timeBuffer, "  %Y    %m    %d", "   ", (int) it->corrValues[3], it->corrValues[2]);
                fprintf(out, "%s%19.12lE", timeBuffer, it->corrValues[0]);
                fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
            }
        }
        return;
	case GEOT:      //"D-UTC A0,A1,T,W,S,U"  (in GEOSTATIONARY NAV version V210)
		for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); ++it) {
            if (it->corrType == TIMC_SBUT) {
                for (i = 0; i < 2; i++) {
                    fprintf(out, "%19.12lE", it->corrValues[i]);
                }
                fprintf(out, "%7d%5d  S%-2.2d %2d ", (int) it->corrValues[2], (int) it->corrValues[3], (int) it->corrValues[5], (int) it->corrValues[4]);
                fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
            }
        }
        return;
	case TIMC :		//"*TIME SYSTEM CORR"		GNSS nav V304
		for (vector<CORRECTION>::iterator it = corrections.begin(); it != corrections.end(); ++it) {
            if (isTimeCorrection(it->corrType)) {
                switch (it->corrType) {
                    case TIMC_GPUT: cnsId = 'G'; break;
                    case TIMC_GLUT:
                    case TIMC_GLGP: cnsId = 'R';
                        it->corrValues[1] = 0;
                        it->corrValues[2] = 0;  //T reference time zero for GLONASS
                        it->corrValues[3] = 0;  //W reference week idem
                        break;
                    case TIMC_GAGP:
                    case TIMC_GAUT: cnsId = 'E'; break;
                    case TIMC_BDUT: cnsId = 'C'; break;
                    case TIMC_QZUT:
                    case TIMC_QZGP: cnsId = 'J'; break;
                    case TIMC_IRUT:
                    case TIMC_IRGP: cnsId = 'I'; break;
                    case TIMC_SBUT: cnsId = 'S'; break;
                    default: cnsId = '?';
                }
				fprintf(out, "%-4.4s %17.10lE%16.9lE%7d%5d %s %2d ",
                        valueLabel(it->corrType).c_str(),
                        it->corrValues[0], it->corrValues[1],
                        (int) it->corrValues[2],
                        (int) it->corrValues[3],
                        desTimeCorrSource(timeBuffer, cnsId, (int) it->corrValues[5]),
                        (int) it->corrValues[4]);
				fprintf(out, "%-20s\n", valueLabel(labelId).c_str());
            }
		}
		return;
	case EOH :		//"END OF HEADER"
		fprintf(out, "%60c", ' ');
		break;
	default:
		return;
	}
	fprintf(out, "%-20.20s\n", valueLabel(labelId).c_str());
	#undef PRINT_SYSREC
}

/**printSatObsValues prints a line or lines with observable values of the first satellite in epochObs.
 * If the number of observables to print is greather than the maximum number of observable values to be printed
 * in one line, one or several continuation lines would be necessary.
 * After printing observation data of this first satellite, they are removed from the storage.
 * It is assumed that values in the observables storage  belong to the same epoch and are sorted by system,
 * satellite PRN and observable type.
 *
 * @param out the already open print stream where RINEX epoch data will be printed
 * @param ver the RINEX version being printed
 * @return true if they remain observables belonging to the current epoch, false when no data remains to print.
 */
bool RinexData::printSatObsValues(FILE* out, RINEXversion ver) {
	double valueToPrint;
	int lli;
    vector <SatObsData>::iterator itobs;
	if (epochObs.empty()) return false;
	//satellite data to print are those of the firts satellite in epochObs
	int sysToPrint = epochObs[0].sysIndex;
	int satToPrint = epochObs[0].satellite;
    int maxPerLine;         //maximum observables to print per line
    switch (ver) {
        case V210: maxPerLine = 5; break;
        default: maxPerLine = 999;  // 999 = practically unlimited
    }
    bool printObs;          //a flag computed to know if current obsType shall be printed or not
    int nObsPrinted = 0;    //number of observables already printed in the current line
    //for all observables of the given system, print values if printable and data available
    for (int i = 0; i < systems[sysToPrint].obsTypes.size(); i++) {
        //check is value for obsType in position i shall be printed or not
        printObs = systems[sysToPrint].obsTypes[i].prt;
        //Check if there are data to print and belongs to the system, satellite and obsType
        itobs = epochObs.begin();
        if (!epochObs.empty() && (itobs->sysIndex == sysToPrint) && (itobs->satellite == satToPrint) && (itobs->obsTypeIndex == i)) {
            if (printObs) {
                //normal case: there are data for this observable and shall be printed
                valueToPrint = itobs->obsValue;
                lli = itobs->lossOfLock;
                //adjust measurements out of range in the RINEX format 14.3f
                while (valueToPrint > MAXOBSVAL) {valueToPrint -= 1.E9; lli |= 1;}
                while (valueToPrint < MINOBSVAL) {valueToPrint += 1.E9; lli |= 1;}
                fprintf(out, "%14.3lf", valueToPrint);
                if (lli == 0) fprintf(out, " ");
                else fprintf(out, "%1d", lli);
                if (itobs->strength == 0) fprintf(out, " ");
                else fprintf(out, "%1d", itobs->strength);
                nObsPrinted++;
            } else {
                plog->warning(msgIgnObservable
                              + to_string((long double) epochTimeTag)
                              + msgComma + string(1,systems[sysToPrint].system) + to_string((long long) satToPrint)
                              + msgComma + string(systems[sysToPrint].obsTypes[itobs->obsTypeIndex].id));
            }
            epochObs.erase(epochObs.begin());    //remove printed data
        } else if (printObs) {
            //there are no data for this observable, but shall be printed
            fprintf(out, "%14.3lf  ", (double) 0.0);
            nObsPrinted++;
        } else continue;    //this obsType has no data and is not printable
        if ((nObsPrinted % maxPerLine) == 0) {
            fprintf(out, "\n");
            nObsPrinted = 0;
        }
    }
    if ((nObsPrinted % maxPerLine) != 0) fprintf(out, "\n");
    return !epochObs.empty();
}

/**readHdLineData reads a line from input RINEX file identifying the header line type, extracting data contained and storing them into the class members.
 * If line header data is well formated, label is flagged as having data. If error is detected in data format, label is flagged as NOT having data
 * 
 * @param  input the already open print stream where RINEX line header will be read
 * @return the RINEX label identification, NOLABEL if the line has not a valid RINEX lable, DONTMATCH if the label is not valid for the stated RINEX version, or LASTONE if EOF found.
 * @throws error string with message describing it.
 */
RinexData::RINEXlabel RinexData::readHdLineData (FILE* input) {
	///a macro to read continuation lines of a given header record (identified through its label)
	#define READ_CONT_LINE(GIVEN_LABEL, BLANK_SPACE) \
				if (fgets(lineBuffer, sizeof lineBuffer, input) == NULL) return LASTONE;	\
				if (checkLabel(lineBuffer) != GIVEN_LABEL) {	\
					plog->warning(valueLabel(GIVEN_LABEL, msgContExp + string(lineBuffer+61, 20)));	\
					return GIVEN_LABEL; \
				}	\
				if (strrchr(lineBuffer, ' ') < lineBuffer+BLANK_SPACE) {	\
					plog->warning(valueLabel(GIVEN_LABEL, msgFmtCont));	\
					return GIVEN_LABEL;	\
				}
	///a macro log the given error and return
	#define LOG_ERR_AND_RETURN(ERROR_STR) \
			{ \
				plog->warning(valueLabel(labelId, msgWrongFormat) + ERROR_STR); \
				return labelId; \
			}

    char lineBuffer[100], aChar;
	RINEXlabel labelId;
	PRNobsnum prnobs;
	string error, aStr;
	int i, j, k, n, year, month, day, hour, minute;
	double second, aDouble;
	bool readOK;
	vector<string> strList;		//a working list of strings (for observations, satellites, etc.)
	vector<string> obsTypeIds;	//a list of observable types in V300 format
	vector<int> anIntLst;		//a working list of integer
	CORRECTION aCorrection;

	if (readRinexRecord(lineBuffer, sizeof lineBuffer, input))  return LASTONE;
	labelId = checkLabel(lineBuffer);
	switch (labelId) {
	case NOLABEL:	//No label found
		plog->warning (msgNoLabel + string(lineBuffer, 20));
		return NOLABEL;
	case DONTMATCH:
		plog->warning (string(lineBuffer+61, 20) + msgWrongLabel);
		return DONTMATCH;
	case VERSION:	//"RINEX VERSION / TYPE"
		//extract TYPE. In V210: O {N,G,H}. In V304 N, O
		fileType = lineBuffer[20];
		fileTypeSfx = string(lineBuffer+21, 19);
		//extract Satellite System: V210= ' ' G R S T M; V300= G R E J C S M
		sysToPrintId = lineBuffer[40];
		systemIdSfx = string(lineBuffer+41, 19);
		//extract and verify version, and set values as per V304
		if(sscanf(lineBuffer, "%9lf", &aDouble) !=1) aDouble = 0;
		if ((aDouble >= 2) && (aDouble < 3)) {
			inFileVer = V210;
			if (aDouble != 2.1) plog->warning(valueLabel(VERSION, msgProcessV210));
			//store VERSION parameters as per V304
			switch (fileType) {
			case 'O':   //in V210 observation GPS
				if (sysToPrintId == ' ') {
                    sysToPrintId = 'G';
					fileTypeSfx = ":GPS";
				}
				break;
			case 'N':   //in V210 navigation GPS
				sysToPrintId = 'G';
				systemIdSfx = ":GPS";
				break;
			case 'G':   //in V210 navigarion GLONASS
				fileType = 'N';
				sysToPrintId = 'R';
				systemIdSfx = ":GLONASS";
				break;
			case 'H':   //in V210 navigation SBAS
				fileType = 'N';
				sysToPrintId = 'S';
				systemIdSfx = ":SBAS";
				break;
			default:
				throw string("This version only process Observation or Navigation files");
			}
		}
		else if ((aDouble >= 3) && (aDouble < 4)) {
			inFileVer = V304;
			if (aDouble != 3.04) plog->warning(valueLabel(VERSION, msgProcessV304));
		}
		else {
			plog->warning(valueLabel(VERSION, msgProcessTBD));
			inFileVer = VTBD;
		}
		//set systems for navigation files because they do not have SYS / OBS record
		if (fileType == 'N' && sysToPrintId != 'M') {
		    strList.clear();
		    systems.push_back(GNSSsystem(sysToPrintId, strList));
		}
		plog->finer(valueLabel(VERSION, to_string((long double) aDouble)) + msgSlash + string(1,fileType) + msgSlash + string(1,sysToPrintId));
		break;
	case RUNBY:		//"PGM / RUN BY / DATE"
		pgm = string(lineBuffer, 20);
		runby = string(lineBuffer + 20, 20);
		date = string(lineBuffer + 40, 20);
		plog->finer(valueLabel(RUNBY, pgm + msgSlash + runby));
		break;
	case COMM:		//"COMMENT"
		//the comment read is inserted as a new label (header record) after the lastRecordSet (last record read)
		//it is used the LABELdata constructor for COMM records
		lastRecordSet = labelDef.insert(lastRecordSet + 1, LABELdata(string(lineBuffer, 60)));
		plog->finer(valueLabel(COMM, string(lineBuffer, 60)));
		return COMM;
	case MRKNAME:	//"MARKER NAME"
		markerName = string(lineBuffer, 60);
		plog->finer(valueLabel(MRKNAME, markerName)); 
		break;
	case MRKNUMBER:	//"MARKER N"
		markerNumber = string(lineBuffer, 20);
		plog->finer(valueLabel(MRKNUMBER, markerNumber)); 
		break;
	case MRKTYPE:	//"MARKER TYPE"
		markerType = string(lineBuffer, 20);
		plog->finer(valueLabel(MRKTYPE, markerType)); 
		break;
	case AGENCY:	//"OBSERVER / AGENCY"
		observer = string(lineBuffer, 20);
		agency = string(lineBuffer + 20, 40);
		plog->finer(valueLabel(AGENCY, observer + msgSlash + agency));
		break;
	case RECEIVER:	//"REC # / TYPE / VERS
		rxNumber = string(lineBuffer, 20);
		rxType = string(lineBuffer + 20, 20);
		rxVersion = string(lineBuffer + 40, 20);
		plog->finer(valueLabel(RECEIVER, rxNumber + msgSlash + rxType + msgSlash + rxVersion));
		break;
	case ANTTYPE:	//"ANT # / TYPE"
		antNumber = string(lineBuffer, 20);
		antType = string(lineBuffer + 20, 20);
		plog->finer(valueLabel(ANTTYPE, antNumber + msgSlash + antType));
		break;
	case APPXYZ:	//"APPROX POSITION XYZ"
		if(sscanf(lineBuffer, "%14lf%14lf%14lf", &aproxX, &aproxY, &aproxZ) != 3) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(APPXYZ, to_string((long double) aproxX) + msgSlash + to_string((long double) aproxY) + msgSlash + to_string((long double) aproxZ)));
		break;
	case ANTHEN:		//"ANTENNA: DELTA H/E/N"
		if(sscanf(lineBuffer, "%14lf%14lf%14lf", &antHigh, &eccEast, &eccNorth) != 3) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(ANTHEN, to_string((long double) antHigh) + msgSlash + to_string((long double) eccEast) + msgSlash + to_string((long double) eccNorth)));
		break;
	case ANTXYZ:		//"ANTENNA: DELTA X/Y/Z"	V300
		if(sscanf(lineBuffer, "%14lf%14lf%14lf", &antX, &antY, &antZ) != 3) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(ANTXYZ, to_string((long double) antX) + msgSlash + to_string((long double) antY) + msgSlash + to_string((long double) antZ)));
		break;
	case ANTPHC:		//"ANTENNA: PHASECENTE"		V300
		antPhSys = lineBuffer[0];
		antPhCode = string(lineBuffer+2, 3);
		if(sscanf(lineBuffer+5, "%9lf%14lf%14lf", &antPhNoX, &antPhEoY, &antPhUoZ) != 3) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(ANTPHC, string(&antPhSys, 1) + msgSlash + antPhCode + msgSlash + to_string((long double) antPhNoX) + msgSlash + to_string((long double) antPhEoY) + msgSlash + to_string((long double) antPhUoZ)));
		break;
	case ANTBS:			//"ANTENNA: B.SIGHT XYZ"	V300
		if(sscanf(lineBuffer, "%14lf%14lf%14lf", &antBoreX, &antBoreY, &antBoreZ) != 3) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(ANTBS, to_string((long double) antBoreX) + msgSlash + to_string((long double) antBoreY) + msgSlash + to_string((long double) antBoreZ)));
		break;
	case ANTZDAZI:		//"ANTENNA: ZERODIR AZI"	V300
		if(sscanf(lineBuffer, "%14lf", &antZdAzi) != 1) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(ANTZDAZI, to_string((long double) antZdAzi)));
		break;
	case ANTZDXYZ:		//"ANTENNA: ZERODIR XYZ"	V300
		if(sscanf(lineBuffer, "%14lf%14lf%14lf", &antZdX, &antZdY, &antZdZ) !=3) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(ANTZDXYZ, to_string((long double) antZdX) + msgSlash + to_string((long double) antZdY) + msgSlash + to_string((long double) antZdZ)));
		break;
	case COFM :			//"CENTER OF MASS XYZ"		V300
		if(sscanf(lineBuffer, "%14lf%14lf%14lf", &centerX, &centerY, &centerZ) !=3) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(COFM) + to_string((long double) centerX) + msgSlash + to_string((long double) centerY) + msgSlash + to_string((long double) centerZ));
		break;
	case WVLEN:			//"WAVELENGTH FACT L1/2"	V210
		if(sscanf(lineBuffer, "%6d%6d", &i, &j) != 2) LOG_ERR_AND_RETURN(string())
		if((sscanf(lineBuffer+12, "%6d", &k) == 0) || (k == 0)) {
			//it is the default wavelength factor header line
			if (wvlenFactor.empty()) wvlenFactor.push_back(WVLNfactor(i,j));
			else {
				wvlenFactor[0].wvlenFactorL1 = i;
				wvlenFactor[0].wvlenFactorL2 = j;
			}
		} else {
			//it is a wavelength factor header line with satellite numbers (up to 7)
			if (k >= 7) LOG_ERR_AND_RETURN(msgNumsat7)
			for (i = 0, n = 18; i < k; i++, n += 6)
				strList.push_back(string(lineBuffer+n+3, 3));
			wvlenFactor.push_back(WVLNfactor(i, j, strList));
		}
		plog->finer(valueLabel(WVLEN, to_string((long long) i) + msgSlash + to_string((long long) j) + ":" + to_string((long long) strList.size())));
		break;
	case TOBS:		//"# / TYPES OF OBSERV"		V210
		if((sscanf(lineBuffer, "%6d", &k) == 0) || (k == 0)) LOG_ERR_AND_RETURN(string())
		if(sysToPrintId == 'T') LOG_ERR_AND_RETURN(msgTransit);
		n = k;	//expected number of types. If n>9 there will be continuation line(s)
		while (n > 0) {  //get V210 types and convert them to V300 notation
		    strList.clear();
			strList = getTokens(string(lineBuffer+6, 54), ' ');
			for (vector<string>::iterator it = strList.begin(); it != strList.end(); ++it) {
			    for (i=0; !v2obsTypes[i].empty() && (v2obsTypes[i].compare(*it) != 0); i++);
				if (v2obsTypes[i].empty()) plog->warning(valueLabel(TOBS, (*it) + msgObsNoTrans));
				else obsTypeIds.push_back(v3obsTypes[i]);
			}
			n -= 9;
			if (n > 0) {	//read a continuation line and verify its label
				READ_CONT_LINE(TOBS, 6)
			}
		}
		if (k != obsTypeIds.size()) plog->warning(valueLabel(TOBS, msgMisCode));
		//	store data on observable types
		if (sysToPrintId == 'M') {
		    //when data come from multiple systems, add obsTypeIds for each system in V210
			systems.push_back(GNSSsystem('G', obsTypeIds));
			systems.push_back(GNSSsystem('R', obsTypeIds));
			systems.push_back(GNSSsystem('S', obsTypeIds));
		}
		else systems.push_back(GNSSsystem(sysToPrintId, obsTypeIds));
		plog->finer(valueLabel(TOBS, to_string((long long) k) + msgTypes));
		break;
	case SYS :		//"SYS / # / OBS TYPES"		V300
		if (lineBuffer[0] == ' ') LOG_ERR_AND_RETURN(msgSysUnk)
		if((sscanf(lineBuffer+3, "%6d", &k) == 0) || (k == 0)) LOG_ERR_AND_RETURN(msgNumTypesNo)
		n = k;	//expected number of types. If n>13 there will be continuation line(s)
		while (n > 0) {
            strList.clear();
			strList = getTokens(string(lineBuffer+6, 54), ' ');	//extract type codes from the line
			obsTypeIds.insert(obsTypeIds.end(), strList.begin(), strList.end());
			n -= 13;
			if (n > 0) {	//read a continuation line and verify its label
				READ_CONT_LINE(SYS, 6)
			}
		}
		if (k != obsTypeIds.size()) plog->warning(valueLabel(SYS, msgMisCode));
		//store data on observable types
		systems.push_back(GNSSsystem(lineBuffer[0], obsTypeIds));
		plog->finer(valueLabel(SYS, to_string((long long) k) + msgTypes));
		break;
	case SIGU :		//"SIGNAL STRENGTH UNIT"
		signalUnit = string(lineBuffer, 20);
		plog->finer(valueLabel(SIGU, signalUnit));
		break;
	case INT :		//"INTERVAL"
		if(sscanf(lineBuffer, "%10lf", &obsInterval) != 1) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(INT, to_string((long double) obsInterval)));
		break;
	case TOFO :		//"TIME OF FIRST OBS"
		if(sscanf(lineBuffer, "%6d%6d%6d%6d%6d%13lf", &year, &month, &day, &hour, &minute, &second) != 6) LOG_ERR_AND_RETURN(string())
		//use date to obtain first observable time
		getWeekTowGPSdate (year, month, day, hour, minute, second, firstObsWeek, firstObsTOW);
		obsTimeSys = getSysId(string(lineBuffer + 48, 3));
		plog->finer(valueLabel(TOFO, to_string((long long) firstObsWeek) + msgSlash + to_string((long double) firstObsTOW)));
		break;
	case TOLO :		//"TIME OF LAST OBS"
		if(sscanf(lineBuffer, "%6d%6d%6d%6d%6d%13lf", &year, &month, &day, &hour, &minute, &second) != 6) LOG_ERR_AND_RETURN(string())
		//use date to obtain last obsrvation time. Time system ignored: same system as per TOFO assumed.
		getWeekTowGPSdate (year, month, day, hour, minute, second, lastObsWeek, lastObsTOW);
		plog->finer(valueLabel(TOLO, to_string((long long) lastObsWeek) + msgSlash + to_string((long double) lastObsTOW)));
		break;
	case CLKOFFS :	//"RCV CLOCK OFFS APPL"
		if(sscanf(lineBuffer, "%6d", &rcvClkOffs) != 1) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(CLKOFFS, to_string((long long) rcvClkOffs)));
		break;
	case DCBS :		//"SYS / DCBS APPLIED"
		if ((n = systemIndex(lineBuffer[0])) < 0) LOG_ERR_AND_RETURN(msgSysUnk)
		dcbsApp.push_back(DCBSPCVSapp(n, string(lineBuffer + 1, 17), string(lineBuffer + 20, 40)));
		plog->finer(valueLabel(DCBS, string(" for sys ") + string(1, lineBuffer[0])));
		break;
	case PCVS :		//"SYS / PCVS APPLIED"
		if ((n = systemIndex(lineBuffer[0])) < 0) LOG_ERR_AND_RETURN(msgSysUnk)
		pcvsApp.push_back(DCBSPCVSapp(n, string(lineBuffer + 1, 17), string(lineBuffer + 20, 40)));
		plog->finer(valueLabel(DCBS, string(" for sys ") + string(1, lineBuffer[0])));
		break;
	case SCALE :	//"SYS / SCALE FACTOR"
		if ((i = systemIndex(lineBuffer[0])) < 0) LOG_ERR_AND_RETURN(msgSysUnk)
		if(sscanf(lineBuffer+2, "%4d", &k) == 0) LOG_ERR_AND_RETURN(msgNoScale)
		if(sscanf(lineBuffer+8, "%2d", &j) == 1) {
			n = j;
			//n is the expected number of types. If n>12 there will be continuation line(s)
			while (n > 0) {
                strList.clear();
				strList = getTokens(string(lineBuffer+10, 48), ' ');	//extract type codes from the line
				obsTypeIds.insert(obsTypeIds.end(), strList.begin(), strList.end());
				n -= 12;
				if (n > 0) {	//read a continuation line and verify its label
					READ_CONT_LINE(SCALE, 10)
				}
			}
		}
		if (j != obsTypeIds.size()) plog->warning(valueLabel(SCALE, msgMisCode));
		//store data on observable types
		obsScaleFact.push_back(OSCALEfact(i, k, obsTypeIds));
		plog->finer(valueLabel(SCALE, to_string((long long) k) + " scale for " + to_string((long long) j) + msgTypes));
		break;
	case PHSH :		//"SYS / PHASE SHIFTS"
		if ((i = systemIndex(lineBuffer[0])) < 0) LOG_ERR_AND_RETURN(msgSysUnk)
		if(sscanf(lineBuffer+6, "%8lf", &aDouble) == 0) LOG_ERR_AND_RETURN(msgNoCorrection)
		if(sscanf(lineBuffer+8, "%2d", &j) == 1) {
			n = j;
			//n is the expected number of types. If n>10 there will be continuation line(s)
			while (n > 0) {
                strList.clear();
				strList = getTokens(string(lineBuffer+18, 40), ' ');	//extract type codes from the line
				obsTypeIds.insert(obsTypeIds.end(), strList.begin(), strList.end());
				n -= 10;
				if (n > 0) {	//read a continuation line and verify its label
					READ_CONT_LINE(PHSH, 18)
				}
			}
		}
		if (j != obsTypeIds.size()) plog->warning(valueLabel(PHSH, msgMisCode));
		//store data on observable types
		phshCorrection.push_back(PHSHcorr(i, string(lineBuffer+2, 3), aDouble, obsTypeIds));
		plog->finer(valueLabel(PHSH, msgPhPerType + to_string((long double) aDouble) + msgComma + to_string((long long) j)));
		break;
	case GLSLT :	//"GLONASS SLOT / FRQ #"
		if(sscanf(lineBuffer+8, "%2d", &j) == 1) {
			n = j;	//j is the expected number of satellites. If j>8 there would be continuation line(s)
			k = 4;	//k is the index in lineBuffer to the satelite data to extract
			while (n > 0) {
				if(sscanf(lineBuffer+k+1, "%2d", &j) == 0) plog->warning(valueLabel(GLSLT,msgNoSlot));
				else if(sscanf(lineBuffer+k+4, "%2d", &i) == 0) plog->warning(valueLabel(GLSLT, msgNoFreq));
				else gloSltFrq.push_back(GLSLTfrq(j, i));
				n--;
				k += 6;
				if (k > 46) {	//read a continuation line and verify its label
					READ_CONT_LINE(GLSLT, 4)
				}
			}
		}
		if (j != gloSltFrq.size()) plog->warning(valueLabel(GLSLT, msgMisSlots));
		plog->finer(valueLabel(GLSLT, to_string((long long) j) + msgSlots));
		break;
	case LEAP :		//"LEAP SECONDS"
		if(sscanf(lineBuffer, "%6d", &n) != 1) LOG_ERR_AND_RETURN(string())
        //if V304 additional data may be present
        if (memcmp(lineBuffer + 24, "BDS", 3) == 0) aChar = 'C';    //set system time identifier
        else aChar = 'G';
        if (isBlank(lineBuffer + 6, 6)) i = 0;      //set i = leap delta LSF
        else i = stoi(string(lineBuffer + 6, 6));
        if (isBlank(lineBuffer + 12, 6)) j = 0;     //set j = week LSF
        else j = stoi(string(lineBuffer + 12, 6));
        if (isBlank(lineBuffer + 18, 6)) k = 0;    //set k = day LSF
        else k = stoi(string(lineBuffer + 18, 6));
        //check if this data is already stored
        readOK = true;  //it means here that leapSecs record shall be added
        for (vector<LEAPsecs>::iterator it = leapSecs.begin(); readOK && it != leapSecs.end(); it++) {
            if ((it->sysId == aChar) && (n == it->secs)) readOK = false;
        }
        if (readOK) {
            leapSecs.push_back(LEAPsecs(n, i, j, k, aChar));
            plog->finer(valueLabel(LEAP, to_string((long long) n)));
        }
		break;
	case SATS :		//"# OF SATELLITES"
		if(sscanf(lineBuffer, "%6d", &numOfSat) != 1) LOG_ERR_AND_RETURN(string())
		plog->finer(valueLabel(SATS, to_string((long long) numOfSat)));
		break;
	case PRNOBS :	//"PRN / # OF OBS"
		//get the list with the number of observables
		for (i = 0; i < 9; i++) {
			if(sscanf(lineBuffer+6+i*6, "%6d", &k) == 0) break;
			anIntLst.push_back(k);
		}
		if ((lineBuffer[3] != ' ') && (sscanf(lineBuffer+4, "%2d", &k) == 1)) {
			//It is the first line for the given satellite
			prnobs.sysPrn = lineBuffer[3];
			prnobs.satPrn = k;
			prnobs.obsNum = anIntLst;
			prnObsNum.push_back(prnobs);
		} else {
			//It is a continuation line of the last PRNOBS read
			if (prnObsNum.empty()) LOG_ERR_AND_RETURN(msgWrongCont)
			prnObsNum.back().obsNum.insert(prnObsNum.back().obsNum.end(), anIntLst.begin(), anIntLst.end());
		}
		plog->finer(valueLabel(PRNOBS, string(1, prnObsNum.back().sysPrn) + msgSlash + to_string((long long) prnObsNum.back().obsNum.size())));
		break;
	case IONA:
        //TODO implement reader
	case IONB:
	    //TODO implement reader
        LOG_ERR_AND_RETURN(string("NOT IMPLEMENTED"))
	case IONC :		//"IONOSPHERIC CORR"	GNSS nav V304
		aCorrection.corrType = findLabelId(lineBuffer);
		if (aCorrection.corrType == IONC_GAL) {
			n = 3;
			aCorrection.corrValues[3] = 0.0;
		}
		else n = 4;
		readOK = true;
		for (i = 0; readOK && i < n; i++) {
			if(sscanf(lineBuffer+5+i*12, "%12lf", &aCorrection.corrValues[i]) != 1) {
			    readOK = false;
			}
		}
        if(readOK && sscanf(lineBuffer+5+4*12+1, "%c%d", &aChar, &n) == 2) {
		    aCorrection.corrValues[4] = (aChar - 'A') * 60 * 60;  //refTime in seconds, as TOW
			aCorrection.corrValues[5] = n;
		}
        else readOK = false;
		if (readOK) {
			//set has data flag in the equivalent V2 labels
			switch (aCorrection.corrType) {
				case IONC_GPSA:
					setLabelFlag(IONA);
					break;
				case IONC_GPSB:
					setLabelFlag(IONB);
					break;
				default:
					break;
			}
			corrections.push_back(aCorrection);
			plog->finer(valueLabel(IONC, msgDataRead));
        }
        else plog->warning(valueLabel(IONC, msgErrCorr));
		break;
	case DUTC:
        //TODO implement reader
	case CORRT:
        //TODO implement reader
	case GEOT:
        //TODO implement reader
        LOG_ERR_AND_RETURN(string("NOT IMPLEMENTED"))
	case TIMC :		//"TIME SYSTEM CORR"	GNSS nav V304
        readOK = true;
        aCorrection.corrType = findLabelId(lineBuffer);
		if (aCorrection.corrType == TIMC_GLUT) {    //in GLONASS only -TauC is included
            if(sscanf(lineBuffer+5, "%17lf", &aCorrection.corrValues[0]) != 1) readOK = false;
			aCorrection.corrValues[1] = 0.0;
		} else {    //in other system get a0, a1, T and W
		    if(sscanf(lineBuffer+5+i*17, "%17lf%16lf%7d%5d", &aCorrection.corrValues[0], &aCorrection.corrValues[1], &i, &j) != 4) readOK = false;
            aCorrection.corrValues[2] = i;
            aCorrection.corrValues[3] = j;
		}
		if((sscanf(lineBuffer+5+17+16+5+5, " %5c%d", lineBuffer, &n) != 2)) readOK = false;
        if (readOK) {
            aCorrection.corrValues[4] = n;
            aCorrection.corrValues[5] = idTimeCorrSource(lineBuffer);
			//set has data flag in the equivalent V2 labels
			switch (aCorrection.corrType) {
				case TIMC_GPUT:
					setLabelFlag(DUTC);
					break;
				case TIMC_GLUT:
					setLabelFlag(CORRT);
					break;
				case TIMC_SBUT:
					setLabelFlag(GEOT);
					break;
				default:
					break;
			}
			corrections.push_back(aCorrection);
        	plog->finer(valueLabel(TIMC, msgDataRead));
        }
        else plog->warning(valueLabel(TIMC, msgErrCorr));
		break;
	case EOH :		//"END OF HEADER"
		plog->finer(valueLabel(EOH, msgFound));
		break;
	default:
		throw msgInternalErr;
		break;
	}
	setLabelFlag(labelId);
	return labelId;
#undef READ_CONT_LINE
#undef LOG_ERR_AND_RETURN
}

/**readRinexRecord reads a line from the RINEX input containing a header line or observation record
 * It removes the EOL, appends blanks and adds the string null delimiter.
 * Empty lines are skipped.
 *
 * @param rinexRec a pointer to a record buffer
 * @param recSize the size in bytes of the record buffer
 * @param  input the already open print stream where RINEX line will be read
 * @return true if EOF happens when reading, false otherwise
 */
bool RinexData::readRinexRecord(char* rinexRec, int recSize, FILE* input) {
	size_t obsLen;
	do {
		if (fgets(rinexRec, recSize, input) == NULL) return true;
		obsLen = strlen(rinexRec) - 1;
		memset(rinexRec + obsLen, ' ', recSize - obsLen);
	} while (isBlank(rinexRec, recSize-1));
	return false;
}

/**isSatSelected checks if the given system and satellite are selected.
 * Note that for a system selected an empty list of satellites is considered as ALL SELECTED
 * @param sysIx the given system index in the systems vector 
 * @param sat the given satellite number 
 * @return true when the given system is selected and the satellite is in the list of selected ones or the list is empty, false otherwise
 */
bool RinexData::isSatSelected(int sysIx, int sat) {
    if (sysIx < 0) return false;
    if (!systems[sysIx].selSystem) return false;
	if (systems[sysIx].selSat.empty()) return true;
	for (vector<int>::iterator its = systems[sysIx].selSat.begin(); its != systems[sysIx].selSat.end(); its++)
		if ((*its) == sat) return true;
	return false;
}

/**systemIndex provides the system index in the systems vector for a given system code
 * 
 * @param sysCode the one character system code (G, R, S, E, ...)
 * @return the index of the given system code in the systems vector, or -1 if it is not in the vector
 */
int RinexData::systemIndex(char sysId) {
	for (int i = 0; i < (int) systems.size(); i++)
		if (systems[i].system == sysId) return i;
	return -1;
}

/**getSysIndex get the index in the systems vector of a given system identification
 *
 * @param  sysId the system identification character (G, S, R, ....)
 * @return the index inside the vector system for it
 * @throws error string with the related message
 */
unsigned int RinexData::getSysIndex(char sysId) {
    int inx;
    if ((inx = systemIndex(sysId)) >= 0) return (unsigned int) inx;
    throw msgSysUnk + string(1, sysId);
}

/**getSysDes provides the system description for a given system code
 * 
 * @param s the one character system code (G, R, S, E, ...)
 * @return the descriptive sufix of the given system code
 */
string RinexData::getSysDes(char s) {
    for (vector<SYSdescript>::iterator it = sysDescript.begin() ; it != sysDescript.end(); ++it)
        if (it->sysId == s) return it->sysDes;
	return string();
}

/**getSysId provides the system identification for a given system time description
 *
 * @param s the system time description
 * @return the system identifier of the given system time description
 */
char RinexData::getSysId(string des) {
    for (vector<SYSdescript>::iterator it = sysDescript.begin() ; it != sysDescript.end(); ++it)
        if (des.compare(it->timeDes) == 0) return it->sysId;
    return ' ';
}

/**getSysTimeDes provides the system time description for a given system code
 *
 * @param s the one character system code (G, R, S, E, ...)
 * @return the system time identifier of the given system code
 */
string RinexData::getTimeDes(char s) {
    for (vector<SYSdescript>::iterator it = sysDescript.begin() ; it != sysDescript.end(); ++it)
        if (it->sysId == s) return it->timeDes;
    return string();
}

/**setSuffixes set values in descriptive suffixes of VERSION record
 * values are set according version and file type
 */
void RinexData::setSuffixes() {
    const string desOBS = "BSERVATION DATA";
    if (version == V210) {
        switch (fileType) {
            case 'O':   //observation
                fileTypeSfx = desOBS;
                systemIdSfx = getSysDes(sysToPrintId);
                break;
            case 'N':
                fileTypeSfx = "AVIGATION GPS DATA";
                systemIdSfx = getSysDes('G');
                break;
            case 'G':
                fileTypeSfx = "LONASS NAVIGATION";
                systemIdSfx = getSysDes('R');
                break;
            case 'L':
                fileTypeSfx = " GALILEO NAVIGATION";
                systemIdSfx = getSysDes('E');
                break;
            case 'B':
                fileTypeSfx = " SBAS NAVIGATION";
                systemIdSfx = getSysDes('S');
                break;
            default:
                break;
        }
    } else { //it is assumed V310
        switch (fileType) {
            case 'O':   //observation
                fileTypeSfx = desOBS;
                break;
            case 'N':
                fileTypeSfx = "AVIGATION DATA";
                break;
            default:
                break;
        }
        systemIdSfx = getSysDes(sysToPrintId);
    }
}

/**isIonoCorrection checks if the given correction identifier is a ionospheric correction or not.
 *
 * @param corr the correction identifier to be checked
 * @return true if is ionospheric, false otherwise
 */
bool RinexData::isIonoCorrection(RINEXlabel corr) {
    switch (corr) {
        case IONC_GAL:   ///< "GAL "
        case IONC_GPSA:  ///< "GPSA"
        case IONC_GPSB:  ///< "GPSB"
        case IONC_QZSA:  ///< "QZSA"
        case IONC_QZSB:  ///< "QZSB"
        case IONC_BDSA:  ///< "BDSA"
        case IONC_BDSB:  ///< "BDSB"
        case IONC_IRNA:  ///< "IRNA"
        case IONC_IRNB:  ///< "IRNB"
            return true;
        default:
            return false;
    }
}

/**isTimeCorrection checks if the given correction identifier is a time correction or not.
 *
 * @param corr the correction identifier to be checked
 * @return true if is time correction, false otherwise
 */
bool RinexData::isTimeCorrection(RINEXlabel corr) {
    switch (corr) {
        case TIMC_GPUT:  ///< "GPUT"
        case TIMC_GLUT:  ///< "GLUT"
        case TIMC_GAUT:  ///< "GAUT"
        case TIMC_BDUT:  ///< "BDUT"
        case TIMC_QZUT:  ///< "QZUT"
        case TIMC_IRUT:  ///< "IRUT"
        case TIMC_SBUT:  ///< "SBUT"
        case TIMC_GLGP:  ///< "GLGP"
        case TIMC_GAGP:  ///< "GAGP"
        case TIMC_QZGP:  ///< "QZGP"
        case TIMC_IRGP:  ///< "IRGP"
            return true;
        default:
            return false;
    }
}
/**idTimeCorrSource gives the identifier of the time corrections source.
 *The source values can be:
 * - Snn (a MEO system-satellite identification). The identifier is the integer nn
 * - nnn (a SBAS satellite identification broadcasting the MT12). The identifier is the integer nnn
 * - WAAS (the SBAS identification broadcasting the MT17). The identifier is 1000
 * - EGNOS (the SBAS identification broadcasting the MT17). The identifier is 1001
 * - MSAS (the SBAS identification broadcasting the MT17). The identifier is 1002
 * @param buffer the five characters defining the correction source
 * @return 0 if the source cannot be identified, or the source identification otherwise
 */
int RinexData::idTimeCorrSource(char* buffer) {
    int n = 0;
    while ((n < 6) && (*buffer == ' ')) {
        n++;
        buffer++;
    }
    if (n == 6) return 0;
    *(buffer+5) = 0;    //convert to a null terminated string
    if (strcmp(buffer, "WAAS") == 0) return 1000;
    if (strcmp(buffer, "EGNOS") == 0) return 1001;
    if (strcmp(buffer, "MSAS") == 0) return 1002;
    if (strchr("GRECS", *buffer) != NULL) buffer++;
    if (sscanf(buffer, "%d", &n) == 1) return n;
    return 0;
}
/**desTimeCorrSource gives the description of the time corrections source.
 *The description can be (x = blank):
 * - Snnxx (a MEO system-satellite identification). The identifier is the integer nn < 100
 * - Snnnx (a SBAS satellite identification broadcasting the MT12). The identifier is the integer 99 < nnn < 1000
 * - WAASx (the SBAS identification broadcasting the MT17). The identifier is 1000
 * - EGNOS (the SBAS identification broadcasting the MT17). The identifier is 1001
 * - MSASx (the SBAS identification broadcasting the MT17). The identifier is 1002
 * @param buffer the five characters defining the correction source
 * @return 0 if the source cannot be identified, or the source identification otherwise
 */
char* RinexData::desTimeCorrSource(char* buffer, char system, int satNum) {
    if (satNum < 100) sprintf(buffer, "%c%-2.2d  ", system, satNum);
    else if (satNum < 1000) sprintf(buffer, "%c%-3.3d ", system, satNum);
    else if (satNum == 1000) strcpy(buffer, "WAAS ");
    else if (satNum == 1001) strcpy(buffer, "EGNOS");
    else if (satNum == 1002) strcpy(buffer, "MSAS ");
    else strcpy(buffer, "     ");
    return buffer;
}
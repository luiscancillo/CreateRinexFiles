##Introduction 

This project includes the JNI code which implements the interface between the Java developed part of the toRINEX application in charge of the app GUI and GNSS data acquisition, and the C++ code which implements the functionality related to extraction of GNSS observables from data acquired and the generation of the RINEX files.

RINEX is the standard format used to feed with observation and navigation data files the software packages used for computing positioning solutions with high accuracy. 

RINEX data files are not aimed to compute solutions in real time, but to perform post-processing, that is, GNSS/GPS receiver data are first collected into files using the receiver specific format, then converted to standard RINEX data files, and finally processed using, may be, facilities having number-crunching capabilities, and additional data (like data from reference stations available in UNAVCO, EUREF, GDC, IGN, and other data repositories) to allow removal of received data errors. 

A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. An updated document exists also for Version 3.04. 


##Data Files 

The classes provided are used to process or generate the following file types. 


###RINEX observation 

A RINEX observation file is a text file containing a header with data related to the file data acquisition, and sequences of epoch data. For each epoch the observables for each satellite tracked are included. See above reference on RINEX for a detailed description of this file format. 


###RINEX navigation 

A RINEX navigation file is a text file containing a header with data related to the file data acquisition, and satellite ephemeris obtained from the navigation message transmitted inside the navigation satellite signal. See above reference on RINEX for a detailed description of this file format. 

###Android ORD

Observation Raw Data (ORD) files are text files containing observation data records captured by the toRINEX APP during the acquisition process. Data in such records came from the own device GNSS receiver, not from an external one.
It is important to note that not all Android smart phones are able to provide GNSS raw data. See Android developpers web site for information on this subject.

###Android NRD

Navigation Raw Data (NRD)) files are text files containing navigation messages data records captured by the toRINEX APP during the acquisition process. Data in such records came from the own device GNSS receiver, not from an external one.
It is important to note that not all Android smart phones are able to provide GNSS raw data. See Android developpers web site for information on this subject. Even, there are devices that provide observation raw data, but not the navigation messages sent by the satellites.



##C++ Classes and routines

###native-lib

The module native-lib.ccp contains the interface routine to be called from Java to collect data from raw data files (.ORD for Observation Raw Data, and .NRD for Navigation Raw Data) and generate the related RINEX files.

Files are generated taking into account parameters passed from the Java call. To perform that, the common classes GNSSdataFromGRD, RinexData, and Logger are used (see detailed documentation in the CommonClasses project).
 

###RinexData 

The RinexData class defines a data container for the RINEX file header records, epoch observables, and satellite navigation ephemeris. 

The class provides methods: 
- To store RINEX header data and set parameters in the container 
- To store epoch satellite ephemeris 
- To store epoch observables 
- To print RINEX file header 
- To print RINEX ephemeris data 
- To print RINEX epoch observables 

Also it includes methods to read existing RINEX files, to store their data in the container, and to access and print them. 

Methods are provided to set data filtering criteria (system, satellite, observables), and to filter data before printing. 

###GNSSdataFromGRD

This class provides methods to extract RINEX data from GNSS data in ORD and NRD files records. Data extracted are stored in RinexData objects (header data, observations data, etc.) to allow further printing in RINEX file formats.

###Logger 

The Logger class allows recording of tagged messages in a logging file. The class defines a hierarchy of log levels (SEVERE, WARNING, INFO, CONFIG, FINE, FINER or FINEST) and provides methods to set the current log level and log messages at each level. 

Only those messages having level from SEVERE to the current level stated are actually recorded in the log file. 




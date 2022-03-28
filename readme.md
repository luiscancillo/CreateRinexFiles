
This project is part of the toRINEX Android app, which generates files with GNSS observation and navigation data in RINEX format from GPS / GNSS data acquired by the smart smartphone own receiver. Full documentation of this project can be found in the docs/html/index.html directory.

The GUI and data acquisition parts of the toRINEX application has been implemented in Java, being the code and documentation in the repository of the toRINEX project.

The scope of this CreateRinexFiles project is the code and documentation to implement the functionality related to generation of RINEX files from GNSS data acquired.

Such functionality has been implemented in C++ using JNI routines (for the native Java to C++ interface) and predefined C++ classes for generating RINEX files. This code and related documentation can be found in the project CommonClasses, although for convenience it is also included here.

Note: the current repository does not include all files for the whole Android Studio project, which would be available after the on-going reorganisation of the project (or upon request).

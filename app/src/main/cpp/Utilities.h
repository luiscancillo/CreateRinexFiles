/** @file Utilities.h
 * Contains definition of routines used in several places.
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
 *<p>V2.0	|2/2016	|Added functions
 *<p>V3.0   |12/2019|Removed dependency from ctime functions for GPS time computation by adding Modified Julian day computations
 *                  |Added functions and change names of some existing ones
 */
#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <vector>

using namespace std;

vector<string> getTokens (string source, char separator);
bool isBlank (char* buffer, int n);
string strToUpper(string strToConvert);
int getTwosComplement(unsigned int number, int nbits);
int getSigned(unsigned int number, int nbits);
unsigned int reverseWord(unsigned int wordToReverse, int nBits=32);
char getFirstDigit(string intNum, char defChar = ' ');

void formatUTCtime(char* buffer,  size_t bufferSize, const char* fmt);
double getUTCinstant(int year, int month, int day, int hour, int min, double sec);
int getWeek(double instant);
double getTow(double instant);
double getInstant(int week, double tow);
int getMjd(int year, int month, int day);
int getMjdFromGPST(int week, double tow);
void mjdTodate(int mjd, int &year, int &month, int &day);

void formatGPStime (char* buffer,  size_t bufferSize, const char* fmtYtoM, const char * fmtSec, int week, double tow); //convert to printable format the given GPS time
int getWeekGPSdate (int year, int month, int day, int hour, int min, double sec); //computes GPS weeks from the GPS ephemeris (6/1/1980) to a given date
double getTowGPSdate (int year, int month, int day, int hour, int min, double sec); //computes the TOW for a given date
void getWeekTowGPSdate (int year, int month, int day, int hour, int min, double sec, int & week, double & tow);
double getInstantGPSdate (int year, int month, int day, int hour, int min, double sec); //compute instant in seconds from the GPS ephemeris (6/1/1980) to a given date and time
int getWeekGNSSinstant (double secs); //computes weeks from the ephemeris to a given instant
double getTowGNSSinstant (double secs); //computes the TOW for a given instant
double getInstantGNSStime (int week, double tow); //compute instant seconds from the ephemeris to a given GNSS time (week and tow)
#endif
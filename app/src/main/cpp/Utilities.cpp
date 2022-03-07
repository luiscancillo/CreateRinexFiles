/** @file Utilities.cpp
 * Contains the implementation of routines used in several places.
 */
#include "Utilities.h"
#include <sstream>
#include <algorithm>
#include <ctype.h>
#include <time.h>
#include <math.h>

/**getTokens gets tokens from a string separated by the given separator
 *
 * @param source a string to be split into tokens
 * @param separator the character used to separate tokens
 * @return a vector of strings containing the extracted tokens
 */
vector<string> getTokens (string source, char separator) {
    string strBuf;
    stringstream ss(source);
    vector<string> tokensFound;
    while (getline(ss, strBuf, separator))
        if(strBuf.size() > 0) tokensFound.push_back(strBuf);
    return tokensFound;
}

/**isBlank checks if in the given char buffer all chars are blanks
 *
 * @param buffer the array of chars to check
 * @param n the length to check
 * @return true if all n chars are blanks, false otherwise
 */
bool isBlank (char* buffer, int n) {
    while (n-- > 0) if (*(buffer++) != ' ')  return false;
    return true;
}

/**strToUpper gets a string containing the given string with characters converted to upper case.
 *
 * @param strToConvert the string to convert to upper case
 * @return the converted string
 */
string strToUpper(string strToConvert) {
    string strConverted;
    for (string::iterator p = strToConvert.begin(); p!= strToConvert.end(); p++) strConverted += string(1,toupper(*p));
    return strConverted;
}

/**getTwosComplement converts a two's complement number represented with the given number of bits to an int having the same value.
 * The given number passed as unsigned int is a bit stream where the right nbits represent an integer in two's complement, that is:
 * - if number is in the range 0 to 2^(nbits-1) is positive. Its value in a sizeof(int)*8 (usually 32) bits representation is the same.
 * - if number is in the range 2^(nbits-1) to 2^(nbits)-1, is negative and must be converted subtracting 2^(nbits)
 *
 * @param number the bit pattern with the number to be interpreted
 * @param nbits	the number of significant bits in the pattern, Shall be equal or smaller than the number of bits in an int
 * @return the value of the number (its int representation)
 */
int getTwosComplement(unsigned int number, int nbits) {
    int value = number;
    if ((nbits >= sizeof(number)*8) || (nbits <= 0)) return value;	//the conversion is imposible or not necessary
    if (number < ((unsigned int) 1 << (nbits-1))) return value;	    //the number is positive, it do not need conversion
    return value - (1 << nbits);
}

/**getSigned converts a signed representation of a number depicted with a given number of bits to a standard int (in two's complement).
 * The given number passed as unsigned int is a bit stream where the right nbits represent a signed integer, that is:
 * - if the most significant bit is 0, the number is positive. If 1, the number is negative
 * - the rest of bits contain the absolute value of the number
 *
 * @param number the bits pattern with the number to be interpreted
 * @param nbits	the number of significative bits in the pattern
 * @return the value of the number (its int representation)
 */
int getSigned(unsigned int number, int nbits) {
    unsigned int signMask;
    int value = number;
    if ((nbits <= sizeof(number)*8) && (nbits > 0)) {		//the conversion is possible
        signMask =  0x01U << (nbits-1);
        if ((number & signMask) != 0) value = - (int)(number & ~signMask); //the number is negative
    }
    return value;
}

/*reverseWord swap LSB given bits in the given word.
 *It is assumed that nBits <= sizeof (unsigned int)
 *
 * @param wordToReverse	word containing the bits to reverse
 * @param nBits	the number of bits in the word to reverse
 * @return a word with the bits to reverse in reverse order
*/
unsigned int reverseWord(unsigned int wordToReverse, int nBits) {
    unsigned int reversed = 0;
    while (nBits > 0) {
        reversed <<= 1;
        reversed |= wordToReverse & 0x01;
        wordToReverse >>= 1;
        nBits--;
    }
    return reversed;
}

/**getFirstDigit gets the fist digit of an integer number in the given string, or the default char if the string is not a number
 *
 * @param intNum contains the integer number
 * @param defChar the deafault char
 * @return the fist digit
 */
char getFirstDigit(string intNum, char defChar) {
    int n;
    try {
        n = stoi(intNum);
        if (n < 0) return defChar;
        return to_string(n).at(0);

    } catch (invalid_argument) {
        return defChar;
    } catch (out_of_range) {
        return defChar;
    }
}

/**formatUTCtime gives text calendar data of the current UTC computer time using the format provided (as per strftime).
 *
 * @param buffer the text buffer where calendar data are placed
 * @param bufferSize of the text buffer in bytes
 * @param fmt the format to be used for conversion, as per strftime
 */
void formatUTCtime(char* buffer, size_t bufferSize, const char* fmt) {
    //get GMT time and format it as requested
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime (buffer, bufferSize, fmt, timeinfo);
}

/**getUTCinstant computes the UTC time instant: seconds from the UNIX ephemeris (1/1/1970 00:00:00.0) to the given UTC date and time
 *
 * Remark: some implementations of ctime do not take into account leap seconds!!
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @return the seconds from 0h of 6/1/1980 to the given date
 */
double getUTCinstant(int year, int month, int day, int hour, int min, double sec) {
    //get integer and fractional part of sec
    double intSec;
    double fractSec = modf(sec, &intSec);
    //set UTC start epoch: 1/1/1970 00:00:00
    struct tm utcEpoch = { 0 };
    utcEpoch.tm_year = 70;
    utcEpoch.tm_mon = 0;
    utcEpoch.tm_mday = 1;
    utcEpoch.tm_hour = 0;
    utcEpoch.tm_min = 0;
    utcEpoch.tm_sec = 0;
    utcEpoch.tm_isdst = -1;
    //set given date
    struct tm date = { 0 };
    date.tm_year = year - 1900;
    date.tm_mon = month - 1;
    date.tm_mday = day;
    date.tm_hour = hour;
    date.tm_min = min;
    date.tm_sec = 0 + (int) intSec;
    date.tm_isdst = -1;
    //compute time difference in integer seconds
    intSec = difftime(timegm(&date), timegm(&utcEpoch));
    return  intSec + fractSec;
}

/**getWeekNumber compute number of weeks from the ephemeris to a given instant in seconds
 *
 * @param instant seconds from of the ephemeris
 * @return the weeks from time start to the given instant
 */
int getWeekNumber(double instant) {
    return (int) (instant / 604800.0);	// 7d * 24h * 60min * 60sec = 604800
}

/**getTow compute the Time Of Week seconds from the beginning of week to a given instant in seconds
 *
 * @param instant seconds from the beginnig of ephemeris
 * @return the TOW for the given instant
 */
double getTow(double instant) {
    return instant - (double) getWeekGNSSinstant (instant) * 604800.0;	// 7d * 24h * 60min * 60sec = 604800
}

/**getInstant compute the instant in seconds for given time given as week number and and tow
 *
 * @param week the week number
 * @param tow the time of week
 * @return the seconds from the time start to the given time stated as week number and tow
 */
double getInstant(int week, double tow) {
    return (double) week * 604800.0 + tow; // 7d * 24h * 60min * 60sec = 604800
}

/**getMjd returns the Modified Julian Day for a given calendar date.
 *
 * Valid for Gregorian dates from 17-Nov-1858.
 * Adapted from sci.astro FAQ.
 *
 * @param year is the calendar year
 * @param month is the month number in the range 1 to 12
 * @param day is the day of month in the range 1 to 31
 * @return the Modified Julian Day number
 */
int getMjd(int year, int month, int day) {
    return
            367 * year
            - 7 * (year + (month + 9) / 12) / 4
            - 3 * ((year + (month - 9) / 7) / 100 + 1) / 4
            + 275 * month / 9
            + day
            + 1721028
            - 2400000;
}

/**mjdToDate converts the given Modified Julian Day to calendar date.
 *
 * - Assumes Gregorian calendar.
 * - Adapted from Fliegel/van Flandern ACM 11/#10 p 657 Oct 1968.
 * @param mjd is Modified Julian Day to convert
 * @param year is the calendar year
 * @param month is the month number in the range 1 to 12
 * @param day is the day of month in the range 1 to 31
 */

void mjdToDate(int mjd, int *year, int *month, int *day) {
    int j, c, y, m;

    j = mjd + 2400001 + 68569;
    c = 4 * j / 146097;
    j = j - (146097 * c + 3) / 4;
    y = 4000 * (j + 1) / 1461001;
    j = j - 1461 * y / 4 + 31;
    m = 80 * j / 2447;
    *day = j - 2447 * m / 80;
    j = m / 11;
    *month = m + 2 - (12 * j);
    *year = 100 * (c - 49) + y + j;
}

/**getMjdFromGPST gets Modified Julian Day from GPS time stated as week and time of week
 *
 * It ignores UTC leap seconds.
 * @param week is the full week number, without rollover
 * @param tow is the time of week, or seconds from the begining of the week
 * @return the Modified Julian Day
 */
int getMjdFromGPST(int week, double tow) {
    double dow;     //day of week
    modf(tow / 86400., &dow); //get day of week from tow
    return 44244        //is getMjd(1980, 1, 6), the GPS epoch
           + week * 7
           + (int) dow;
}

/**formatGPStime formats a GPS time giving text GPS calendar data using time formats provided.
 *
 * @param buffer the text buffer where calendar data are placed
 * @param bufferSize of the text buffer in bytes
 * @param fmtYtoM the format to be used for year, month, day, hour and minute (all int), as per strftime
 * @param fmtSec the format to be used for seconds (a double), as per sprintf. If null, seconds are not printed.
 * @param week the GPS week from 6/1/1980
 * @param tow the GPS time of week, or seconds from the beginning of the week
 */
void formatGPStime (char* buffer,  size_t bufferSize, const char* fmtYtoM, const char* fmtSec, int week, double tow) {
    #define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
    double d;
    //use tm only for formatting time data
    //mktime is avoided due to adjust it introduces (dayligth, UTC rollover, etc.)
    struct tm gpsEphe = { 0 };
    mjdToDate(getMjdFromGPST(week, tow), &gpsEphe.tm_year, &gpsEphe.tm_mon, &gpsEphe.tm_mday);
    int daysMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (_ISLEAP(gpsEphe.tm_year)) daysMonth[1] = 29;
    //set tm values in the expected range
    gpsEphe.tm_year -= 1900;
    gpsEphe.tm_mon -= 1;
    while (tow >= 86400.) tow -= 86400.;    //remove days of week already computed
    for (gpsEphe.tm_hour = 0; tow >= 3600; gpsEphe.tm_hour++) tow -= 3600;
    for (gpsEphe.tm_min = 0; tow >= 60; gpsEphe.tm_min++) tow -= 60;
    for (int i = 0; i < gpsEphe.tm_mon; i++) gpsEphe.tm_yday += daysMonth[i];
    gpsEphe.tm_yday += gpsEphe.tm_mday - 1;
    gpsEphe.tm_wday = getMjdFromGPST(week, tow) % 7;  //MJD 0, the 17th nov. 1858, was sunday
    //format data
    strftime (buffer, bufferSize, fmtYtoM, &gpsEphe);
    if ((*fmtSec) != 0) {
        size_t n = strlen(buffer);
        snprintf(buffer + n, bufferSize - n, fmtSec, tow);
    }
}

/**getWeekGPSdate compute number of weeks from the GPS ephemeris (6/1/1980) to a given GPS date
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @return the weeks from 6/1/1980 to the given date
 */
int getWeekGPSdate (int year, int month, int day, int hour, int min, double sec) {
    return (int) (getInstantGPSdate (year, month, day, hour, min, sec) / 604800.0);	// 7d * 24h * 60min * 60sec = 604800
}

/**getTowGPSdate compute the Time Of Week (seconds from the beginning of week at Sunday 00:00h) to a given GPS date
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @return the TOW for the given date
 */
double getTowGPSdate (int year, int month, int day, int hour, int min, double sec) {
    return getInstantGPSdate (year, month, day, hour, min, sec) -
           getWeekGPSdate (year, month, day, hour, min, sec) * 604800.0;	// 7d * 24h * 60min * 60sec = 604800
}

/**getWeekTowGPSdate compute GPS week and tow for a given GPS date
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @param week GPS week to be computed
 * @param tow GPS tow to be computed
 */
void getWeekTowGPSdate (int year, int month, int day, int hour, int min, double sec, int& week, double& tow) {
    sec = getInstantGPSdate(year, month, day, hour, min, sec);
    week = int (sec / 604800.0);
    tow = fmod(sec, 604800.0);
}

/**getInstantGPSdate computes the GPS time instant: seconds from the GPS ephemeris (6/1/1980) to a given GPS date
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @return the seconds from 0h of 6/1/1980 to the given date
 */
double getInstantGPSdate(int year, int month, int day, int hour, int min, double sec) {
    return (getMjd(year, month, day) - 44244) * 86400   //44244 is getMjd(1980, 1, 6), the GPS epoch. 86400 are seconds per day
            + hour * 3600
            + min * 60
            + sec;
}

/**getInstantGNSStime compute instant in seconds from the ephemeris to a given time (week and tow)
 *
 * @param week the week number (continuous, without roll over)
 * @param tow the time of week
 * @return the seconds from the time start to the given time stated as week number and tow
 */
double getInstantGNSStime (int week, double tow) {
    return (double) week * 604800.0 + tow; // 7d * 24h * 60min * 60sec = 604800
}

/**getWeekGNSSinstant compute number of weeks from the ephemeris to a given GNSS instant in seconds
 *
 * @param secs seconds from of the ephemeris
 * @return the weeks from time start to the given instant
 */
int getWeekGNSSinstant (double secs) {
    return (int) (secs / 604800.0);	// 7d * 24h * 60min * 60sec = 604800
}

/**getTowGNSSinstant compute the Time Of Week seconds from the begining of week for a given GNSS instant
 *
 * @param secs seconds from the beginnig of ephemeris
 * @return the TOW for the given instant
 */
double getTowGNSSinstant (double secs) {
    return secs - (double) getWeekGNSSinstant (secs) * 604800.0;	// 7d * 24h * 60min * 60sec = 604800
}

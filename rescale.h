#ifndef RESCALE_H
#define RESCALE_H

#ifdef UINT16
typedef unsigned short raw_t;
const char *RESCALE_DTYPE = "16-bit unsigned integer";
#else
typedef float raw_t;
const char *RESCALE_DTYPE = "32-bit floating point";
#endif

/* version */
const char *RESCALE_NAME = "rescale";
const char *RESCALE_VERSION = "0.0.4";
const char *RESCALE_AUTHORS = "Dr. Richard Boardman, Dr. Neil O'Brien, Nick Hale";
const char *RESCALE_MUVIS = "µ-VIS X-ray Imaging Centre";
const char *RESCALE_COPYRIGHT = "Copyright (c) 2016, 2017, 2019 University of Southampton";

/* default values */

#define PROCESSED_SUFFIX ".8bit.scaled.raw" /* default output suffix */
#define BUFFER_COUNT 100000000 /* default number of elements for read/write buffers */
#define MAX_BUFFER 100000000000 /* the maximum allowable buffer size */
#define DEFAULT_HISTOGRAM_BINS 65536 /* the number of histogram bins */
#define THRESHOLD 0.002 /* values below this or above 1-this will be scaled out */

/* constants */

#define GIBI 1073741824 /* per gibi(byte) */
#define MEBI 1048576 /* per mebi(byte) */

/* traps */
#if defined(_WIN32) || defined(_WIN64) || defined(WINNT) || defined(WIN32) || defined(WIN64) || defined(__WIN32) || defined(__WIN64)
#define WINDOWS
#endif

/* define a few error codes */

#define OK 0
#define ERR_HELP_REQUESTED 1
#define ERR_NOT_ENOUGH_ARGUMENTS 2
#define ERR_ARGUMENTS_BEYOND_RECOGNITION 3
#define ERR_FLOAT_SIZE_NOT_PARTICULARLY_THIRTY_TWO_BIT_FLOATY 4
#define ERR_STUPID_CONSTRAINTS 5
#define ERR_UNREADABLE_FILE_UNSURPRISINGLY_CANNOT_BE_READ 6
#define ERR_FILE_STATS_UNREADABLE_DESPITE_FILE_BEING_READABLE 7
#define ERR_FAILED_TO_OPEN_THE_FILE_DESPITE_EVERYTHING_ELSE 8
#define ERR_BAD_THRESHOLD 9
#define ERR_FAILED_TO_READ_A_VALUE_FROM_AN_OPEN_FILE 10
#define ERR_FAILED_TO_OPEN_VGI_FILE 11

/* function prototypes */

int64_t get_filesize(const char *filename);

void info();

void usage();

int read_vgi(char *vgi_filename, int x, int y, int z);

int read_first_value(char *filename, raw_t *target);

uint64_t find_minmax_values(char *filename, raw_t *minval, raw_t *maxval, uint64_t total_size_read, uint64_t total_size_input, uint64_t bufcount, raw_t *buffer, time_t clk_split);

uint64_t build_histogram(char *filename, uint64_t *histogram, raw_t minval, float bin_factor, uint64_t total_size_read, uint64_t total_size_input, uint64_t bufcount, raw_t *buffer, time_t clk_split);

uint64_t calculate_number_of_values(uint64_t *histogram, int nbins);

void convert_data(char *input_file, char *output_file, raw_t *inbuffer, unsigned char *outbuffer, float lowval, float scalerange, uint64_t buffer_count, uint64_t *total_size_read,  uint64_t *total_size_written,  uint64_t total_size_input);

void strip_ext();
#endif

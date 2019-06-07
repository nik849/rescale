/*
  rescale.c

  Dr. Richard Boardman
  µ-VIS X-ray Imaging Centre
  (c) 2016 University of Southampton

  Rescales one or more 32-bit floating point raw data files against a percentile range
  and outputs the results as a series of 8-bit unsigned char datasets

  Modified 2019 Nick Hale
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <getopt.h>
#include <stdint.h>
#include <inttypes.h>
#include "rescale.h"
#include <errno.h>


/*
   assumes all free input parameters are 32-bit datasets
   output files to be with a suffix indicating they've been procesed

   usage: rescale file1-32bit.raw file2-32bit.raw ... filen-32bit.raw
   (will output file1-...PROCESSED_SUFFIX ... filen-...PROCESSED_SUFFIX)
*/

/* program information */
void info()
{
  printf("%s v%s for %s data types\n", RESCALE_NAME, RESCALE_VERSION, RESCALE_DTYPE);
  printf("%s\n", RESCALE_AUTHORS);
  printf("%s\n", RESCALE_MUVIS);
  printf("%s\n", RESCALE_COPYRIGHT);
#ifdef WINDOWS
  printf("***WARNING*** Compiled on Windows. Here be dragons.\n");
#else
  printf("Compiled on not-Windows. Behaviour within normal bounds.\n");
#endif
}

/* write out the usage */
void usage()
{
  printf("Usage: rescale [options] inputfile1.raw inputfile2.raw ... inputfilen.raw\n");
  printf("where available [options] are:\n");
  printf(" -h\tPrints help\n");
  printf(" -t n\tSets saturation threshold to n. For example, a value of 0.123 would mean that the first\n");
  printf("\tand last 12.3%% of values are considered outside the range for scaling and any value\n");
  printf("\tin this range is set to 0 or 255 (8-bit low- and high-value respectively)\n");
  printf(" -b n\tBuffer size (input and output) in n elements. Setting this to e.g. 100000 will\n");
  printf("\tuse 400000 bytes for the input buffer (raw_t) and another 100000 bytes for the\n");
  printf("\toutput (write) buffer. Higher values are recommended for performance reasons.\n");
  printf("\tDefault value is %d\n", BUFFER_COUNT);
  printf(" -s STR\tSets the output suffix to STR. Output files will have the same name as the input\n");
  printf("\tfiles, with STR appended to them. For example, if STR is .8bit.out, the file foo.raw\n");
  printf("\twill become foo.raw.8bit.out. Default value is %s\n", PROCESSED_SUFFIX);
  printf(" -n n\tSets the number of histogram bins to n. Setting a value less than 1 will fail.\n");
  printf("\tDefault value is %d\n", DEFAULT_HISTOGRAM_BINS);
  printf(" -a\t*NEW* Sets output name to Auto - this looks for the corresponding .vgi file in the\n");
  printf("\tsame directory as the .vol and try to extract the size of the volume and append to the\n");
  printf("\toutput filename.");
#ifdef UINT16
  printf("Please note that the %s version will not consider values\n", RESCALE_DTYPE);
  printf("of 0 or 65535 in the scaling - these are known saturated values\n");
#endif
}

int64_t get_filesize(const char *filename)
{
#ifdef WINDOWS
  struct __stat64 st;
  if (_stat64(filename, &st) == 0)
#else
  struct stat st;
  if (stat(filename, &st) == 0)
#endif
    {
      return st.st_size;
    }
  return -1;
}


int read_first_value(char *filename, raw_t *target)
{
  FILE *infile;
  raw_t value;
  int readcount;
  infile = fopen(filename, "rb");
  if (infile == NULL)
    {
      printf("Error opening file %s\n", filename);
      return ERR_FAILED_TO_OPEN_THE_FILE_DESPITE_EVERYTHING_ELSE;
    }

   //printf("sizeof(target) is %d\n", sizeof(target));
  // printf("&value is %u\n", &value);
  // printf("value is %d\n",
 //	 value);
 //  printf("\n\n\n");
  // printf("file is %d\n", infile);
  //if (infile == NULL) { printf("FILE IS NULL\n\n"); }
  //printf("sizeof(target) is %lu\n", sizeof(*target));
  readcount = fread(&value, sizeof(*target),1,infile);
  //printf("READ COUNT\n\n\n");
  // printf("%s:\n",strerror(errno));
  //if (1==0) //if (ferror(infile))

  if (ferror(infile))
    {
        printf("%s:\n",strerror(errno));
      printf("Error reading value from file %s\n", filename);
      return ERR_FAILED_TO_READ_A_VALUE_FROM_AN_OPEN_FILE;
    }
  //   printf("%s:\n",strerror(errno));
    //printf("NOFERROR\n\n\n");
  if (readcount != 1)
    {
      printf("Tried to read one value, instead read %d values. This is weird and not what we want.\n", readcount);
      return ERR_FAILED_TO_READ_A_VALUE_FROM_AN_OPEN_FILE;
    }
  *target = value;
  // if (infile == NULL) { printf("infile is NULL\n"); } else { printf("infile is not NULL\n");}
   //printf("%s\n",strerror(errno));
  // closing the file causes segfaults even if not null -- what (!?)
  fclose(infile);

  return 0;
}


uint64_t find_minmax_values(char *filename, raw_t *minval, raw_t *maxval, uint64_t total_size_read, uint64_t total_size_input, uint64_t bufcount, raw_t *buffer, time_t clk_split)
{
  FILE *infile;
  uint64_t u;
  size_t read_elements;

  printf("Working on file %s\n", filename);
  //printf("minval is %d\n", *minval);
  //printf("minval is %d\n", *maxval);
  infile = fopen(filename, "rb");

  while(!feof(infile))
    {
      read_elements = fread(buffer, sizeof(raw_t), bufcount, infile);
      //	printf("fread done\n");
      total_size_read += read_elements * sizeof(raw_t);
      //	printf("inc done\n");

      printf("Read %" PRIu64 " bytes of %" PRIu64 " (%0.3f of %0.3f GiB, (%0.3f MiB/s), %0.2f%%)",
		 total_size_read,
		 total_size_input,
		 (float)total_size_read / GIBI,
		 (float)total_size_input / GIBI,
		 ((float)total_size_read / MEBI) / (time(NULL)-clk_split),
		 100*(float)total_size_read / (float)total_size_input);

      //	printf("Read %0.3f GiB\n", (float)total_size_read  / GIBI);
      /* with a handful of cores, this can more or less double the performance of this section
	 but generally speaking, I/O will be the bottleneck here by a factor of perhaps 10:1 depending
	 on the data source */
      /* #pragma omp parallel for schedule(static) */
	//printf("extent handle, read elements at %lu\n", read_elements);
      for (u=0; u<read_elements;u++)
	{
	  // printf("min and max at %u and %u\n", *minval, *maxval);
	  if (buffer[u] < *minval) { *minval = buffer[u]; }
	  if (buffer[u] > *maxval) { *maxval = buffer[u]; }
	}
      //printf("outloop\n");
      printf(" - min/max values now %0.4f / %0.4f\r", (float)*minval, (float)*maxval);
    }

  fclose(infile);
  printf("\n");
  return total_size_read;
}

uint64_t build_histogram(char *filename, uint64_t *histogram, raw_t minval, float bin_factor, uint64_t total_size_read, uint64_t total_size_input, uint64_t bufcount, raw_t *buffer, time_t clk_split)
{
  FILE *infile;
  uint64_t u;
  size_t read_elements;
  int bin;
  infile = fopen(filename, "rb");
  printf("Working on file %s\n", filename);

  while(!feof(infile))
    {
      read_elements = fread(buffer, sizeof(raw_t), bufcount, infile);
      total_size_read += read_elements * sizeof(raw_t);
      printf("Read %" PRIu64 " bytes of %" PRIu64 " (%0.3f of %0.3f GiB, (%0.3f MiB/s), %0.2f%%)\r",
	     total_size_read,
	     total_size_input,
	     (float)total_size_read / GIBI,
	     (float)total_size_input / GIBI,
	     ((float)total_size_read / MEBI) / (time(NULL)-clk_split),
	     100*(float)total_size_read / (float)total_size_input);

      for (u = 0; u < read_elements; u++)
	{
#ifdef UINT16
	  /* do not count values of exactly zero for this; skew on Versa reconstructor */
	  if (buffer[u] == 0 || buffer[u] == 65535 ) { continue; }
#endif
	  bin = (int)(bin_factor * (buffer[u] - minval));

	  histogram[bin]++;
	}
    }
  printf("\n");
  return total_size_read;
}

uint64_t calculate_number_of_values(uint64_t *histogram, int nbins)
{
  uint64_t nvals = 0;
  unsigned int i;
  for (i=0; i<nbins; i++)
    {
      nvals += histogram[i];
    }

  return nvals;
}

void convert_data(char *input_file, char *output_file, raw_t *inbuffer, unsigned char *outbuffer, float lowval, float scalerange, uint64_t buffer_count, uint64_t *total_size_read,  uint64_t *total_size_written,  uint64_t total_size_input)
{
  uint64_t u;
  int val;
  FILE *infile, *outfile;
  infile = fopen(input_file, "rb");
  outfile = fopen(output_file, "wb");
  size_t read_elements;

  while(!feof(infile))
    {
      read_elements = fread(inbuffer, sizeof(raw_t), buffer_count, infile);
      *total_size_read += sizeof(raw_t)*read_elements;
      printf("Read %" PRIu64 " bytes of %" PRIu64 " (%0.3f of %0.3f GiB, %0.2f%%)",
	     *total_size_read,
	     total_size_input,
	     (float)(*total_size_read)/GIBI,
	     (float)(total_size_input)/GIBI,
	     100*((float)(*total_size_read)) / (float)total_size_input );
      for (u = 0; u < read_elements; u++)
	{
	  val = (signed int)(255*((inbuffer[u] - lowval)/scalerange));
	  // sanity check and truncate for byte
	  if (val < 0) { val = 0; }
	  else if (val > 255) { val = 255; }
	  outbuffer[u] = (unsigned char)val;
	}

      *total_size_written += read_elements * sizeof(unsigned char);
      fwrite(outbuffer, sizeof(unsigned char), read_elements, outfile);
      printf(" - written %" PRIu64 " bytes (%0.3f GiB)\r", *total_size_written, (float)*total_size_written / GIBI);
    }
  printf("\n");
  fclose(outfile);
  fclose(infile);
}

char *read_update_size_vgi(char *vgifile, int x, int y, int z)
{
  int count = 0;
  char line[256];
  FILE *input_file = fopen(vgifile, "rb");
  int failed = 0;
  char *output_filename = malloc(sizeof(char *) * 1028);
  printf(".vgi name is %s\n", vgifile);
  if (input_file == NULL)
    {
      printf("Error opening .vgi file, check it exists...");
      failed = 1;
      //return ERR_FAILED_TO_OPEN_VGI_FILE;
    }
  if (failed != 0)
  {
    output_filename = PROCESSED_SUFFIX;
  }
  else
  {
    while (fgets(line, sizeof line, input_file) != NULL)
    {
      if (strstr(line, "size =") != NULL) {
        //printf("%s\n", line);
        sscanf(line, "%*[^0123456789]%d%*[^0123456789]%d%*[^0123456789]%d", &x, &y, &z);
        printf("Size will be: %d by %d by %d\n", x, y, z);
        break;
      }
      //printf(line);
      count++;
    }
    fclose(input_file);
    sprintf(output_filename, "%dx%dx%dx8bit.raw", x, y, z);
    printf("Output string set to Auto: %s\n", output_filename);
  }

  return output_filename;
}

void strip_ext(char *fname)
{
    char *end = fname + strlen(fname);

    while (end > fname && *end != '.' && *end != '\\' && *end != '/') {
        --end;
    }
    if ((end > fname && *end == '.') &&
        (*(end - 1) != '\\' && *(end - 1) != '/')) {
        *end = '\0';
    }
}


int main(int argc, char **argv)
{
  int i, opt, a; /* signed int counter, option counter, absolute argument counter */
  raw_t maxval, minval, lowval, highval; /* maximum/minimum raw_t values, and low/high raw_t values computed from histogram */
  float range, scalerange, binsize; /* full range, range for scaling and histogram bin size */
  uint64_t nvals; /* number of values defined across all inputs */
  float pvals, bfac; /* cumulative summation of percentile points across the histogram to find bin value, 'bin factor' */
  float t_low, t_high; /* low and high percentile thresholds */
  uint64_t total_size_input, total_size_read, total_size_written; /* I/O counters */
  raw_t *inbuffer; /* 32-bit raw_t read buffer */
  unsigned char *outbuffer; /* 8-bit unsigned integer write buffer */
  int nbins; /* number of histogram bins */
  uint64_t *histogram; /* collective histogram data */
  time_t clk_start, clk_split; /* performance timers */
  float threshold; /* single threshold value for command-line overriding (prior to t_low/t_high being assigned) */
  int num_input_files; /* number of input files */
  char **input_files; /* names of input files */
  char **output_files; /* names of output files */
  char *processed_suffix; /* suffix for output files */
  uint64_t buffer_count; /* number of elements in a buffer */
  int x, y, z; /* sizes of the volume, read from .vgi file */
  int auto_flag;
  char *vol_file_name;
  /* initialise some values */
  i = 0;
  nvals = 0;
  pvals = 0.0;
  total_size_input = 0;
  total_size_read = 0;
  total_size_written = 0;
  x = 0;
  y = 0;
  z = 0;
  num_input_files = 0;
  buffer_count = BUFFER_COUNT;
  nbins = DEFAULT_HISTOGRAM_BINS;
  threshold = THRESHOLD;
  processed_suffix = malloc(sizeof(char) * (1+strlen(PROCESSED_SUFFIX)));
  snprintf(processed_suffix, sizeof(char)*(1+strlen(PROCESSED_SUFFIX)), "%s", PROCESSED_SUFFIX);
  time(&clk_start);
  auto_flag = 0;
  vol_file_name = malloc(sizeof(char) * 1028);

  /* dump information before we start doing anything */
  info();

  /* sanity check */
  if (sizeof(float) != 4) /* really, I should switch this type out ... */
    {
      printf("Whoops. The size of a float is not 4 bytes. Cowardly refusing to continue. Go see Rich\n");
      return ERR_FLOAT_SIZE_NOT_PARTICULARLY_THIRTY_TWO_BIT_FLOATY;
    }

  /* handle command-line options */
  while ((opt = getopt(argc, argv, "ahb:t:s:n:")) != -1)
    {
      switch(opt)
	{
	case 'h':
	  usage();
	  return ERR_HELP_REQUESTED;
	case 'b':
	  buffer_count = strtoul(optarg, NULL, 10);
	  if (buffer_count == 0)
	    {
	      printf("Buffer size set to zero. Exiting now owing to ridiculous constraints\n");
	      return ERR_STUPID_CONSTRAINTS;
	    }
	  if (buffer_count < 1000)
	    {
	      printf("Warning: buffer count set unreasonably small. Performance will almost certainly be dreadful.\n");
	    }
	  if (buffer_count > MAX_BUFFER)
	    {
	      printf("Warning: requested buffer count of %" PRIu64 " is larger than the maximum count we wish to allow (%" PRIu64 "), so setting buffer_count to %" PRIu64 " elements\n", buffer_count, MAX_BUFFER, MAX_BUFFER);
	      buffer_count = MAX_BUFFER;
	    }
	  break;
	case 't':
	  threshold = atof(optarg);
	  if (threshold < 0.0 || threshold > 0.5)
	    {
	      printf("Threshold should be between 0.0 and 0.5 (0%% and 50%%)\n");
	      return ERR_BAD_THRESHOLD;
	    }
	  break;
	case 's':
	  /* assign a new suffix */
	  free(processed_suffix);
	  processed_suffix = realloc(NULL, sizeof(char)+(1+strlen(optarg)));
	  snprintf(processed_suffix, sizeof(char)*(1+strlen(optarg)), "%s", optarg);
	  printf("Output suffix set to %s\n", processed_suffix);
	  break;
  case 'a':
    /* set the output string to auto, from vgi file */
    auto_flag = 1;
    printf("Will attempt to read size automatically from .vgi file.\n");
    break;
	case 'n':
	  /* set the number of histogram bins */
	  nbins = atoi(optarg);
	  if (nbins < 1)
	    {
	      printf("Number of histogram bins set to %d. Refusing to continue as this is silly\n", nbins);
	      return ERR_STUPID_CONSTRAINTS;
	    }
	  break;
  default:
	  usage();
    return ERR_ARGUMENTS_BEYOND_RECOGNITION;
	}
    }

  /* allocate buffers */
  inbuffer = (raw_t*)malloc(sizeof(raw_t) * buffer_count);
  outbuffer = (unsigned char*)malloc(sizeof(unsigned char*) * buffer_count);

  /* alright, I need to explain this calloc here. There appears to be
     a peculiar bug buried in at least my version of glibc which
     causes this to either segfault or throw up an error 'corrupted
     double-linked list' when this is freed later on.  I couldn't find
     any sensible reasons as to why, or sensible fixes, however I did
     notice that this would only occur when there is an odd number of
     histogram bins. It's not a particularly unlikely edge case so
     I've simply handled it for now, rightly or wrongly, by allocating
     an even number of addresses rather than an odd number if an odd
     number of bins is requested. Cost: sizeof(uint64_t) in
     memory, and no difference in behaviour (other than it no longer
     segfaults) as we're still only working up to nbins in our loop. */
  histogram = malloc(sizeof(uint64_t *) * 1028);//(uint64_t *)malloc(sizeof(uint64_t *) * nbins);
  num_input_files = argc - optind; /* how many input files do we have? */
  //printf("%d\n", num_input_files);
  if (num_input_files < 1)
    {
      printf("Not enough arguments. Please provide the names of one or more ");
#ifdef UINT16
      printf("16");
#else
      printf("32");
#endif
      printf("-bit raw_t raw files\n");
      return ERR_NOT_ENOUGH_ARGUMENTS;
    }

  printf("[Preflight checks: verifying inputs]\n");
  printf("Working on %d input files\n", num_input_files);
  input_files = malloc(num_input_files * sizeof(char *));
  output_files = malloc(num_input_files * sizeof(char *));

  for (i = 0; i<num_input_files; i++)
    {
      a = i + optind; /* absolute argument index */
      if(access(argv[a], R_OK) == -1)
	{
	  printf("%s is not a readable file. Please check and try again\n", argv[a]);
	  return ERR_UNREADABLE_FILE_UNSURPRISINGLY_CANNOT_BE_READ;
	}

      int64_t fsize = get_filesize(argv[a]);
      if (fsize == -1)
	{
	  printf("Unable to read stats of %s\n", argv[a]);
	  return ERR_FILE_STATS_UNREADABLE_DESPITE_FILE_BEING_READABLE;
	}
      if (auto_flag == 1)
  {
    /*do the vgi thing here, for each file */
    //printf("Reading .vgi file for %s\n", argv[a]);
    free(processed_suffix);
    processed_suffix = realloc(NULL, sizeof(char) * 255);
    //snprintf(processed_suffix, 255, "%s", read_update_size_vgi(argv[a], x, y, z));
    strcpy(vol_file_name, argv[a]);
    strip_ext(vol_file_name);
    strcat(vol_file_name, ".vgi");
    processed_suffix = read_update_size_vgi(vol_file_name, x, y, z);
    ///printf("%s\n", processed_suffix);
  }

	  total_size_input += (uint64_t)fsize;
	  printf("Total size to read is now %" PRIu64 " (%0.4f GiB)\n", total_size_input, (float)total_size_input / GIBI);

	  /* add this to the list */
	  input_files[i] = (char *)malloc(sizeof(char) * (5+strlen(argv[a])));
    output_files[i] = (char *)malloc(sizeof(char) * (5+strlen(argv[a])+strlen(processed_suffix)));
	  snprintf(input_files[i], sizeof(char)*(5+strlen(argv[a])), "%s", argv[a]);
	  snprintf(output_files[i], sizeof(char)*(5+strlen(argv[a])+strlen(processed_suffix)), "%s%s", argv[a], processed_suffix);
	  printf("Added file %s to the list of output files\n", output_files[i]);
    }
  printf("\n");

  printf("[Preflight checks: populating initial min/max values and setting saturation threshold]\n");
  /* set the low and high boundaries for saturation threshold */
  t_low = threshold;
  t_high = 1-t_low;

  printf("Saturation threshold set - percentiles between %0.2f%% and %0.2f%% will be considered\n", 100*t_low, 100*t_high);



  // printf("&maxval is %f\n\n\n", &maxval);
  /* read the first value of the first file and assign this to max/minval */
  if (read_first_value(input_files[0], &maxval) != 0)
  {
    return ERR_FAILED_TO_OPEN_THE_FILE_DESPITE_EVERYTHING_ELSE;
  }

  minval = maxval;
  printf("Read first value: maxval is %0.4f, minval is %0.4f\n", (float)maxval, (float)minval);

  time(&clk_split);
  printf("\n[Read pass 1/3: establishing value extents]\n");

  for (i=0; i<num_input_files; i++)
    {
      total_size_read = find_minmax_values(input_files[i], &minval, &maxval, total_size_read, total_size_input, buffer_count, inbuffer, clk_split);
    }

  range = maxval - minval;
  printf("Established min/max values as %0.4f and %0.4f - range is %0.4f\n", (float)minval, (float)maxval, (float)range);
  binsize = range / (float)nbins;
  printf("Using %d histogram bins (bin size = %0.4f)\n", nbins, binsize);
  time(&clk_split);
  total_size_read = 0;
  bfac = ((float)nbins) / range; /* inverted; could overload binsize for inner loop below */

  printf("\n[Read pass 2/3: constructing histogram]\n");
  for (i=0; i<num_input_files; i++)
    {
      total_size_read = build_histogram(input_files[i], histogram, minval, bfac, total_size_read, total_size_input, buffer_count, inbuffer, clk_split);
    }

  nvals = calculate_number_of_values(histogram, nbins);

  /* assign these to sensible defaults in case t_low/high is set silly */
  lowval = minval;
  highval = maxval;

  printf("\n[Finding min/max percentile extents in histogram]\n");

  for (i=0; i<nbins; i++)
    {
     pvals += (float)histogram[i] / (float)nvals;
     if (pvals < t_low) { lowval = (i * binsize) + minval; }
     if (pvals <= t_high) { highval = (i*binsize) + minval; }
   }

  printf("Low value is %0.4f, high value is %0.4f\n", (float)lowval, (float)highval);
  printf("Min value is %0.4f, max value is %0.4f\n", (float)minval, (float)maxval);

 /* now we have our scaling values */
 scalerange = highval - lowval;
 printf("Scaling range is set to %0.4f\n", scalerange);

 /* reset counters */
 total_size_written = 0;
 total_size_read = 0;

 printf("\n[Read pass 3/3: performing conversion and writing output]\n");

 for (i=0; i<num_input_files; i++)
   {
     convert_data(input_files[i], output_files[i], inbuffer, outbuffer, lowval, scalerange, buffer_count, &total_size_read, &total_size_written, total_size_input);
   }

 free(histogram);
 free(outbuffer);
 free(inbuffer);
 for (i = 0; i < num_input_files; i++)
   {
     free(output_files[i]);
     free(input_files[i]);
   }
 free(output_files);
 free(input_files);

 printf("Total processing time was %0.4f minutes\n", (float)(time(NULL)-clk_start)/60.0f);

 return OK;
}

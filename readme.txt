Information about the rescale tool
==================================

The rescale tool takes a number of 32-bit little-endian floating point
binary files, finds the extents of the data across all the files, and
rescales them within some percentile boundaries whilst writing them
out individually as 8-bit files.

It combines two common operations across n files that are performed
post-reconstuction.

What do I need?
===============

One or more reconstructed files in 32-bit floating point format. Note
that if you have used CTPro to reconstruct these, you should have set
it to reconstruct a 32-bit floating point volume (not a 16- or 8-bit
one), and ensured that the scaling is set to None. It cannot be
overemphasised how important that last part is. The option to
automatically rescale the data should be removed entirely from the
software, as there is prima facie no point to this in the context of a
32-bit floating point volume (particularly when the input files for
the data have only 16 bits of fidelity). However, that
notwithstanding, ensure you have your .vol files as 32-bit / floating
point / little endian / no scaling.

That's it.

What will happen?
=================

You should get one or more 8-bit unsigned integer volumes in the same
location as your original 32-bit volumes, with a suffix
.8bit.scaled.raw - so for example, if your original file was called
/path/to/myfile.vol, you will have one additional file called
/path/to/myfile.vol.8bit.scaled.raw

What are the other options?
===========================

You can override that extension, but be careful - it won't prompt you
to overwrite anything that already exists.

You can also adjust the saturation threshold - by default, it is set
to 0.002 (=0.2%), which means that only the middle 100% - (0.2% * 2) =
99.6% of values are considered. Anything falling beyond these will be
set to either 0 or 255 in the 8-bit volume. This is because we don't
want to lose all of our dynamic range if we just have a couple of tiny
bright patches or noise on the image. You could set it to, say, 0.125
and have it use the middle three-quarters (100% - (12.5% * 2)).

You can adjust the number of bins the histogram will use. This will
default to 2^16 = 65536, but you could set it to 256. Setting it to
silly values (e.g. 2, 1, 0, -45, that sort of thing) will either warn
you about being silly, or not warn you about being silly, and probably
give you silly results. Silly in, silly out.

Finally, you can do some performance tuning with the buffer size. This
governs the amount of memory the program will use, so it will maintain
a flat memory footprint. If you set this value to be 1000, then it
will store 1000 floating point values at once, and 1000 8-bit integer
values at once, for a total memory footprint of delta + (1000 *
(sizeof(float) + sizeof(uint8))) = delta + 5000 bytes. If
sizeof(float) != 4 bytes then the program will probably catch this and
exit. Come and see us if you get this, as we're probably interested in
your exotic hardware. Set this value too small, say, 1000, and the
performance will be slow. Set it massive, and you will likely run out
of memory. The default value is 100000000, so the program should run
in around half a gigabyte of system memory.

Can it fail?
============

Yes, for all sorts of reasons. Chiefly, if you had a file full of
garbage, then you could have the maximum value be infinity or
"not-a-number" (Inf/NaN in IEE754 parlance) because that's what
floating point numbers do on a computer. This will cause the scaling
equation to fail, as the established range could be infinity (or
zero), and multiplying by zero, adding infinity or dividing by things
that aren't numbers have weird effects.

Additionally, if you have big- or other-endian data, you have
pre-scaled files (did you set the scaling to be automatic in CTPro?
You did? Wrong move. See above.), or non-floating point or non-32-bit
files... you get the idea. The inputs are strict.

Who made this?
==============

Dr. Richard Boardman, Dr. Neil O'Brien
µ-VIS X-ray Imaging Centre
Copyright (c) 2016 University of Southampton

Summary options
===============

./rescale -h
rescale v0.0.1
Dr. Richard Boardman, Dr. Neil O'Brien
µ-VIS X-ray Imaging Centre
Copyright (c) 2016 University of Southampton
Compiled on not-Windows. Behaviour within normal bounds.
Usage: rescale [options] inputfile1.raw inputfile2.raw ... inputfilen.raw
where available [options] are:
 -h	Prints help
 -t n	Sets saturation threshold to n. For example, a value of 0.123 would mean that the first
	and last 12.3% of values are considered outside the range for scaling and any value
	in this range is set to 0 or 255 (8-bit low- and high-value respectively)
 -b n	Buffer size (input and output) in n elements. Setting this to e.g. 100000 will
	use 400000 bytes for the input buffer (float) and another 100000 bytes for the
	output (write) buffer. Higher values are recommended for performance reasons.
	Default value is 100000000
 -s STR	Sets the output suffix to STR. Output files will have the same name as the input
	files, with STR appended to them. For example, if STR is .8bit.out, the file foo.raw
	will become foo.raw.8bit.out. Default value is .8bit.scaled.raw
 -n n	Sets the number of histogram bins to n. Setting a value less than 1 will fail.
	Default value is 65536

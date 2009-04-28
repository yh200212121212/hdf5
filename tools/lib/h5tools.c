/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Thursday, July 23, 1998
 *
 * Purpose: A library for displaying the values of a dataset in a human
 *  readable format.
 */

#include <stdio.h>
#include <stdlib.h>

#include "h5tools.h"
#include "h5tools_ref.h"
#include "h5tools_str.h"
#include "h5tools_utils.h"
#include "H5private.h"

#define SANITY_CHECK

#define ALIGN(A,Z)  ((((A) + (Z) - 1) / (Z)) * (Z))

/* global variables */
int compound_data;
FILE *rawdatastream; /* should initialize to stdout but gcc moans about it */
int bin_output; /* binary output */
int bin_form; /* binary form */
int region_output; /* region output */

static h5tool_format_t h5tools_dataformat = { 0, /*raw */

"", /*fmt_raw */
"%d", /*fmt_int */
"%u", /*fmt_uint */
"%d", /*fmt_schar */
"%u", /*fmt_uchar */
"%d", /*fmt_short */
"%u", /*fmt_ushort */
"%ld", /*fmt_long */
"%lu", /*fmt_ulong */
NULL, /*fmt_llong */
NULL, /*fmt_ullong */
"%g", /*fmt_double */
"%g", /*fmt_float */

0, /*ascii */
0, /*str_locale */
0, /*str_repeat */

"[ ", /*arr_pre */
",", /*arr_sep */
" ]", /*arr_suf */
1, /*arr_linebreak */

"", /*cmpd_name */
",\n", /*cmpd_sep */
"{\n", /*cmpd_pre */
"}", /*cmpd_suf */
"\n", /*cmpd_end */

", ", /*vlen_sep */
"(", /*vlen_pre */
")", /*vlen_suf */
"", /*vlen_end */

"%s", /*elmt_fmt */
",", /*elmt_suf1 */
" ", /*elmt_suf2 */

"", /*idx_n_fmt */
"", /*idx_sep */
"", /*idx_fmt */

80, /*line_ncols *//*standard default columns */
0, /*line_per_line */
"", /*line_pre */
"%s", /*line_1st */
"%s", /*line_cont */
"", /*line_suf */
"", /*line_sep */
1, /*line_multi_new */
"   ", /*line_indent */

1, /*skip_first */

1, /*obj_hidefileno */
" "H5_PRINTF_HADDR_FMT, /*obj_format */

1, /*dset_hidefileno */
"DATASET %s ", /*dset_format */
"%s", /*dset_blockformat_pre */
"%s", /*dset_ptformat_pre */
"%s", /*dset_ptformat */
1, /*array indices */
1 /*escape non printable characters */
};

static const h5tools_dump_header_t h5tools_standardformat = { "standardformat", /*name */
"HDF5", /*fileebgin */
"", /*fileend */
SUPER_BLOCK, /*bootblockbegin */
"", /*bootblockend */
H5_TOOLS_GROUP, /*groupbegin */
"", /*groupend */
H5_TOOLS_DATASET, /*datasetbegin */
"", /*datasetend */
ATTRIBUTE, /*attributebegin */
"", /*attributeend */
H5_TOOLS_DATATYPE, /*datatypebegin */
"", /*datatypeend */
DATASPACE, /*dataspacebegin */
"", /*dataspaceend */
DATA, /*databegin */
"", /*dataend */
SOFTLINK, /*softlinkbegin */
"", /*softlinkend */
EXTLINK, /*extlinkbegin */
"", /*extlinkend */
UDLINK, /*udlinkbegin */
"", /*udlinkend */
SUBSET, /*subsettingbegin */
"", /*subsettingend */
START, /*startbegin */
"", /*startend */
STRIDE, /*stridebegin */
"", /*strideend */
COUNT, /*countbegin */
"", /*countend */
BLOCK, /*blockbegin */
"", /*blockend */

"{", /*fileblockbegin */
"}", /*fileblockend */
"{", /*bootblockblockbegin */
"}", /*bootblockblockend */
"{", /*groupblockbegin */
"}", /*groupblockend */
"{", /*datasetblockbegin */
"}", /*datasetblockend */
"{", /*attributeblockbegin */
"}", /*attributeblockend */
"", /*datatypeblockbegin */
"", /*datatypeblockend */
"", /*dataspaceblockbegin */
"", /*dataspaceblockend */
"{", /*datablockbegin */
"}", /*datablockend */
"{", /*softlinkblockbegin */
"}", /*softlinkblockend */
"{", /*extlinkblockbegin */
"}", /*extlinkblockend */
"{", /*udlinkblockbegin */
"}", /*udlinkblockend */
"{", /*strblockbegin */
"}", /*strblockend */
"{", /*enumblockbegin */
"}", /*enumblockend */
"{", /*structblockbegin */
"}", /*structblockend */
"{", /*vlenblockbegin */
"}", /*vlenblockend */
"{", /*subsettingblockbegin */
"}", /*subsettingblockend */
"(", /*startblockbegin */
");", /*startblockend */
"(", /*strideblockbegin */
");", /*strideblockend */
"(", /*countblockbegin */
");", /*countblockend */
"(", /*blockblockbegin */
");", /*blockblockend */

"", /*dataspacedescriptionbegin */
"", /*dataspacedescriptionend */
"(", /*dataspacedimbegin */
")", /*dataspacedimend */
};

static const h5tools_dump_header_t * h5tools_dump_header_format;

/* local prototypes */
static int do_bin_output(FILE *stream, hsize_t nelmts, hid_t tid, void *_mem);
static int render_bin_output(FILE *stream, hid_t tid, void *_mem);
static hbool_t h5tools_is_zero(const void *_mem, size_t size);

hsize_t
        h5tools_render_element(FILE *stream, const h5tool_format_t *info,
                h5tools_context_t *ctx/*in,out*/,
                h5tools_str_t *buffer/*string into which to render */,
                hsize_t curr_pos/*total data element position*/,
                unsigned flags, size_t ncols, hsize_t elmt_counter,
                hsize_t i_count/*element counter*/);
hsize_t
        h5tools_dump_region_data_points(hid_t region_space, hid_t region_id,
                FILE *stream, const h5tool_format_t *info,
                h5tools_context_t *ctx/*in,out*/,
                h5tools_str_t *buffer/*string into which to render */,
                hsize_t curr_pos/*total data element position*/,
                unsigned flags, size_t ncols, hsize_t elmt_counter,
                hsize_t i_count/*element counter*/);
hsize_t
        h5tools_dump_region_data_blocks(hid_t region_space, hid_t region_id,
                FILE *stream, const h5tool_format_t *info,
                h5tools_context_t *ctx/*in,out*/,
                h5tools_str_t *buffer/*string into which to render */,
                hsize_t curr_pos/*total data element position*/,
                unsigned flags, size_t ncols, hsize_t elmt_counter,
                hsize_t i_count/*element counter*/);

/* module-scoped variables */
static int h5tools_init_g; /* if h5tools lib has been initialized */
#ifdef H5_HAVE_PARALLEL
static int h5tools_mpi_init_g; /* if MPI_Init() has been called */
#endif /* H5_HAVE_PARALLEL */

/* Names of VFDs */
static const char *drivernames[] = { "sec2", "family", "split", "multi",
#ifdef H5_HAVE_STREAM
        "stream",
#endif /* H5_HAVE_STREAM */
#ifdef H5_HAVE_PARALLEL
        "mpio",
        "mpiposix"
#endif /* H5_HAVE_PARALLEL */
        };

/* This enum should match the entries in the above drivers_list since they
 * are indexes into the drivers_list array. */
enum {
    SEC2_IDX = 0, FAMILY_IDX, SPLIT_IDX, MULTI_IDX
#ifdef H5_HAVE_STREAM
    ,STREAM_IDX
#endif /* H5_HAVE_STREAM */
#ifdef H5_HAVE_PARALLEL
    ,MPIO_IDX
    ,MPIPOSIX_IDX
#endif /* H5_HAVE_PARALLEL */
}    driver_idx;
#define NUM_DRIVERS     (sizeof(drivernames) / sizeof(drivernames[0]))

    /*-------------------------------------------------------------------------
     * Audience:    Public
     * Chapter:     H5Tools Library
     * Purpose:     Initialize the H5 Tools library
     * Description:
     *      This should be called before any other h5tools function is called.
     *      Effect of any h5tools function called before this has been called is
     *      undetermined.
     * Return:
     *      None
     * Programmer:
     *      Albert Cheng, 2000-10-31
     * Modifications:
     *-------------------------------------------------------------------------
     */
void h5tools_init(void) {
    if (!h5tools_init_g) {
        if (!rawdatastream)
            rawdatastream = stdout;

        h5tools_dump_header_format = &h5tools_standardformat;

        h5tools_init_g++;
    }
}

/*-------------------------------------------------------------------------
 * Audience:    Public
 * Chapter:     H5Tools Library
 * Purpose: Close the H5 Tools library
 * Description:
 *      Close or release resources such as files opened by the library. This
 *      should be called after all other h5tools functions have been called.
 *      Effect of any h5tools function called after this has been called is
 *      undetermined.
 * Return:
 *      None
 * Programmer:
 *      Albert Cheng, 2000-10-31
 * Modifications:
 *-------------------------------------------------------------------------
 */
void h5tools_close(void) {
    if (h5tools_init_g) {
        if (rawdatastream && rawdatastream != stdout) {
            if (fclose(rawdatastream))
                perror("closing rawdatastream");
            else
                rawdatastream = NULL;
        }

        /* Clean up the reference path table, if it's been used */
        term_ref_path_table();

        /* Shut down the library */
        H5close();

        h5tools_init_g = 0;
    }
}

/*-------------------------------------------------------------------------
 * Audience:    Private
 * Chapter:     H5Tools Library
 * Purpose: Get a FAPL for a driver
 * Description:
 *      Get a FAPL for a given VFL driver name.
 * Return:
 *      None
 * Programmer:
 *      Quincey Koziol, 2004-02-04
 * Modifications:
 *      Pedro Vicente Nunes, Thursday, July 27, 2006
 *   Added error return conditions for the H5Pset_fapl calls
 *-------------------------------------------------------------------------
 */
static hid_t h5tools_get_fapl(hid_t fapl, const char *driver,
        unsigned *drivernum) {
    hid_t new_fapl; /* Copy of file access property list passed in, or new property list */

    /* Make a copy of the FAPL, for the file open call to use, eventually */
    if (fapl == H5P_DEFAULT) {
        if ((new_fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
            goto error;
    } /* end if */
    else {
        if ((new_fapl = H5Pcopy(fapl)) < 0)
            goto error;
    } /* end else */

    /* Determine which driver the user wants to open the file with. Try
     * that driver. If it can't open it, then fail. */
    if (!strcmp(driver, drivernames[SEC2_IDX])) {
        /* SEC2 driver */
        if (H5Pset_fapl_sec2(new_fapl) < 0)
            goto error;

        if (drivernum)
            *drivernum = SEC2_IDX;
    }
    else if (!strcmp(driver, drivernames[FAMILY_IDX])) {
        /* FAMILY Driver */

        /* Set member size to be 0 to indicate the current first member size
         * is the member size.
         */
        if (H5Pset_fapl_family(new_fapl, (hsize_t) 0, H5P_DEFAULT) < 0)
            goto error;

        if (drivernum)
            *drivernum = FAMILY_IDX;
    }
    else if (!strcmp(driver, drivernames[SPLIT_IDX])) {
        /* SPLIT Driver */
        if (H5Pset_fapl_split(new_fapl, "-m.h5", H5P_DEFAULT, "-r.h5",
                H5P_DEFAULT) < 0)
            goto error;

        if (drivernum)
            *drivernum = SPLIT_IDX;
    }
    else if (!strcmp(driver, drivernames[MULTI_IDX])) {
        /* MULTI Driver */
        if (H5Pset_fapl_multi(new_fapl, NULL, NULL, NULL, NULL, TRUE) < 0)
        goto error;

        if(drivernum)
        *drivernum = MULTI_IDX;
#ifdef H5_HAVE_STREAM
            }
            else if(!strcmp(driver, drivernames[STREAM_IDX])) {
                /* STREAM Driver */
                if(H5Pset_fapl_stream(new_fapl, NULL) < 0)
                goto error;

                if(drivernum)
                *drivernum = STREAM_IDX;
#endif /* H5_HAVE_STREAM */
#ifdef H5_HAVE_PARALLEL
            }
            else if(!strcmp(driver, drivernames[MPIO_IDX])) {
                /* MPI-I/O Driver */
                /* check if MPI has been initialized. */
                if(!h5tools_mpi_init_g)
                MPI_Initialized(&h5tools_mpi_init_g);
                if(h5tools_mpi_init_g) {
                    if(H5Pset_fapl_mpio(new_fapl, MPI_COMM_WORLD, MPI_INFO_NULL) < 0)
                goto error;

            if(drivernum)
                *drivernum = MPIO_IDX;
        } /* end if */
    } else if (!strcmp(driver, drivernames[MPIPOSIX_IDX])) {
        /* MPI-I/O Driver */
        /* check if MPI has been initialized. */
        if(!h5tools_mpi_init_g)
            MPI_Initialized(&h5tools_mpi_init_g);
        if(h5tools_mpi_init_g) {
            if(H5Pset_fapl_mpiposix(new_fapl, MPI_COMM_WORLD, TRUE) < 0)
                goto error;

            if(drivernum)
                *drivernum = MPIPOSIX_IDX;
        } /* end if */
#endif /* H5_HAVE_PARALLEL */
    }
        else {
            goto error;
        }

        return(new_fapl);

        error:
        if(new_fapl != H5P_DEFAULT)
        H5Pclose(new_fapl);
        return -1;
    }

    /*-------------------------------------------------------------------------
     * Audience:    Public
     * Chapter:     H5Tools Library
     * Purpose:     Open a file with various VFL drivers.
     * Description:
     *      Loop through the various types of VFL drivers trying to open FNAME.
     *      If the HDF5 library is version 1.2 or less, then we have only the SEC2
     *      driver to try out. If the HDF5 library is greater than version 1.2,
     *      then we have the FAMILY, SPLIT, and MULTI drivers to play with (and
     *      the STREAM driver if H5_HAVE_STREAM is defined, that is).
     *
     *      If DRIVER is non-NULL, then it will try to open the file with that
     *      driver first. We assume that the user knows what they are doing so, if
     *      we fail, then we won't try other file drivers.
     * Return:
     *      On success, returns a file id for the opened file. If DRIVERNAME is
     *      non-null then the first DRIVERNAME_SIZE-1 characters of the driver
     *      name are copied into the DRIVERNAME array and null terminated.
     *
     *      Otherwise, the function returns FAIL. If DRIVERNAME is non-null then
     *      the first byte is set to the null terminator.
     * Programmer:
     *      Lost in the mists of time.
     * Modifications:
     *      Robb Matzke, 2000-06-23
     *      We only have to initialize driver[] on the first call, thereby
     *      preventing memory leaks from repeated calls to H5Pcreate().
     *
     *      Robb Matzke, 2000-06-23
     *      Added DRIVERNAME_SIZE arg to prevent overflows when writing to
     *      DRIVERNAME.
     *
     *      Robb Matzke, 2000-06-23
     *      Added test to prevent coredump when the file could not be opened by
     *      any driver.
     *
     *      Robb Matzke, 2000-06-23
     *      Changed name from H5ToolsFopen() so it jives better with the names we
     *      already have at the top of this source file.
     *
     *      Thomas Radke, 2000-09-12
     *      Added Stream VFD to the driver[] array.
     *
     *      Bill Wendling, 2001-01-10
     *      Changed macro behavior so that if we have a version other than 1.2.x
     *      (i.e., > 1.2), then we do the drivers check.
     *
     *      Bill Wendling, 2001-07-30
     *      Added DRIVER parameter so that the user can specify "try this driver"
     *      instead of the default behaviour. If it fails to open the file with
     *      that driver, this will fail completely (i.e., we won't try the other
     *      drivers). We're assuming the user knows what they're doing. How UNIX
     *      of us.
     *-------------------------------------------------------------------------
     */
hid_t h5tools_fopen(const char *fname, unsigned flags, hid_t fapl,
        const char *driver, char *drivername, size_t drivername_size) {
    unsigned drivernum;
    hid_t fid = FAIL;
    hid_t my_fapl = H5P_DEFAULT;

    if (driver && *driver) {
        /* Get the correct FAPL for the given driver */
        if ((my_fapl = h5tools_get_fapl(fapl, driver, &drivernum)) < 0)
            goto done;

        H5E_BEGIN_TRY
            {
                fid = H5Fopen(fname, flags, my_fapl);
            }H5E_END_TRY;

        if (fid == FAIL)
            goto done;

    }
    else {
        /* Try to open the file using each of the drivers */
        for (drivernum = 0; drivernum < NUM_DRIVERS; drivernum++) {
            /* Get the correct FAPL for the given driver */
            if ((my_fapl = h5tools_get_fapl(fapl, drivernames[drivernum], NULL))
                    < 0)
                goto done;

            H5E_BEGIN_TRY
                {
                    fid = H5Fopen(fname, flags, my_fapl);
                }H5E_END_TRY;

            if (fid != FAIL)
                break;
            else {
                /* Close the FAPL */
                H5Pclose(my_fapl);
                my_fapl = H5P_DEFAULT;
            } /* end else */
        }
    }

    /* Save the driver name */
    if (drivername && drivername_size) {
        if (fid != FAIL) {
            strncpy(drivername, drivernames[drivernum], drivername_size);
            drivername[drivername_size - 1] = '\0';
        }
        else {
            /*no file opened*/
            drivername[0] = '\0';
        }
    }

    done: if (my_fapl != H5P_DEFAULT)
        H5Pclose(my_fapl);
    return fid;
}

/*-------------------------------------------------------------------------
 * Audience:    Public
 * Chapter:     H5Tools Library
 * Purpose:     Count the number of columns in a string.
 * Description:
 *      Count the number of columns in a string. This is the number of
 *      characters in the string not counting line-control characters.
 * Return:
 *      On success, returns the width of the string. Otherwise this function
 *      returns 0.
 * Programmer:
 *       Robb Matzke, Tuesday, April 27, 1999
 * Modifications:
 *-------------------------------------------------------------------------
 */
static size_t h5tools_ncols(const char *s) {
    register size_t i;

    for (i = 0; *s; s++)
        if (*s >= ' ')
            i++;

    return i;
}

/*-------------------------------------------------------------------------
 * Audience:    Public
 * Chapter:     H5Tools Library
 * Purpose:     Emit a simple prefix to STREAM.
 * Description:
 *      If /ctx->need_prefix/ is set then terminate the current line (if
 *      applicable), calculate the prefix string, and display it at the start
 *      of a line.
 * Return:
 *      None
 * Programmer:
 *      Robb Matzke, Monday, April 26, 1999
 * Modifications:
 *      Robb Matzke, 1999-09-29
 * If a new prefix is printed then the current element number is set back
 * to zero.
 *      pvn, 2004-07-08
 * Added support for printing array indices:
 *  the indentation is printed before the prefix (printed one indentation
 *  level before)
 *-------------------------------------------------------------------------
 */
static void h5tools_simple_prefix(FILE *stream, const h5tool_format_t *info,
        h5tools_context_t *ctx, hsize_t elmtno, int secnum) {
    h5tools_str_t prefix;
    h5tools_str_t str; /*temporary for indentation */
    size_t templength = 0;
    int i, indentlevel = 0;

    if (!ctx->need_prefix)
        return;

    memset(&prefix, 0, sizeof(h5tools_str_t));
    memset(&str, 0, sizeof(h5tools_str_t));

    /* Terminate previous line, if any */
    if (ctx->cur_column) {
        fputs(OPT(info->line_suf, ""), stream);
        putc('\n', stream);
        fputs(OPT(info->line_sep, ""), stream);
    }

    /* Calculate new prefix */
    h5tools_str_prefix(&prefix, info, elmtno, ctx->ndims, ctx->p_min_idx,
            ctx->p_max_idx, ctx);

    /* Write new prefix to output */
    if (ctx->indent_level >= 0) {
        indentlevel = ctx->indent_level;
    }
    else {
        /*
         * This is because sometimes we don't print out all the header
         * info for the data (like the tattr-2.ddl example). If that happens
         * the ctx->indent_level is negative so we need to skip the above and
         * just print out the default indent levels.
         */
        indentlevel = ctx->default_indent_level;
    }

    /* when printing array indices, print the indentation before the prefix
     the prefix is printed one indentation level before */
    if (info->pindex) {
        for (i = 0; i < indentlevel - 1; i++) {
            fputs(h5tools_str_fmt(&str, 0, info->line_indent), stream);
        }
    }

    if (elmtno == 0 && secnum == 0 && info->line_1st)
        fputs(h5tools_str_fmt(&prefix, 0, info->line_1st), stream);
    else if (secnum && info->line_cont)
        fputs(h5tools_str_fmt(&prefix, 0, info->line_cont), stream);
    else
        fputs(h5tools_str_fmt(&prefix, 0, info->line_pre), stream);

    templength = h5tools_str_len(&prefix);

    for (i = 0; i < indentlevel; i++) {
        /*we already made the indent for the array indices case */
        if (!info->pindex) {
            fputs(h5tools_str_fmt(&prefix, 0, info->line_indent), stream);
            templength += h5tools_str_len(&prefix);
        }
        else {
            /*we cannot count the prefix for the array indices case */
            templength += h5tools_str_len(&str);
        }
    }

    ctx->cur_column = ctx->prev_prefix_len = templength;
    ctx->cur_elmt = 0;
    ctx->need_prefix = 0;

    /* Free string */
    h5tools_str_close(&prefix);
    h5tools_str_close(&str);
}

/*-------------------------------------------------------------------------
 * Audience:    Public
 * Chapter:     H5Tools Library
 * Purpose:     Prints NELMTS data elements to output STREAM.
 * Description:
 *      Prints some (NELMTS) data elements to output STREAM. The elements are
 *      stored in _MEM as type TYPE and are printed according to the format
 *      described in INFO. The CTX struct contains context information shared
 *      between calls to this function. The FLAGS is a bit field that
 *      indicates whether the data supplied in this call falls at the
 *      beginning or end of the total data to be printed (START_OF_DATA and
 *      END_OF_DATA).
 * Return:
 *      None
 * Programmer:
 *      Robb Matzke, Monday, April 26, 1999
 * Modifications:
 *  Robb Matzke, 1999-06-04
 * The `container' argument is the optional dataset for reference types.
 *
 *  Robb Matzke, 1999-09-29
 * Understands the `per_line' property which indicates that every Nth
 * element should begin a new line.
 *
 *      Robb Matzke, LLNL, 2003-06-05
 *      Do not dereference the memory for a variable-length string here.
 *      Deref in h5tools_str_sprint() instead so recursive types are
 *      handled correctly.
 *
 *      Pedro Vicente Nunes, The HDF Group, 2005-10-19
 *        pass to the prefix in h5tools_simple_prefix the total position
 *        instead of the current stripmine position i; this is necessary
 *        to print the array indices
 *        new field sm_pos in h5tools_context_t, the current stripmine element position
 *-------------------------------------------------------------------------
 */
void 
h5tools_dump_simple_data(FILE *stream, const h5tool_format_t *info,
        hid_t container, h5tools_context_t *ctx/*in,out*/, unsigned flags,
        hsize_t nelmts, hid_t type, void *_mem) {
    unsigned char *mem = (unsigned char*) _mem;
    hsize_t        i;         /*element counter  */
    char          *s;
    char          *section;   /*a section of output  */
    int            secnum;    /*section sequence number */
    size_t         size;      /*size of each datum  */
    hid_t          f_type;
    hid_t          region_space;
    hid_t          region_id;
    size_t         ncols = 80; /*available output width */
    h5tools_str_t  buffer;    /*string into which to render */
    int            multiline; /*datum was multiline  */
    hsize_t        curr_pos;  /* total data element position   */
    hsize_t        elmt_counter = 0;/*counts the # elements printed.
                                     *I (ptl?) needed something that
                                     *isn't going to get reset when a new
                                     *line is formed. I'm going to use
                                     *this var to count elements and
                                     *break after we see a number equal
                                     *to the ctx->size_last_dim.   */

    /* binary dump */
    if (bin_output) {
        do_bin_output(stream, nelmts, type, _mem);
    } /* end if */
    else {
        /* setup */
        HDmemset(&buffer, 0, sizeof(h5tools_str_t));
        size = H5Tget_size(type);

        if (info->line_ncols > 0)
            ncols = info->line_ncols;

        /* pass to the prefix in h5tools_simple_prefix the total position
         * instead of the current stripmine position i; this is necessary
         * to print the array indices
         */
        curr_pos = ctx->sm_pos;

        h5tools_simple_prefix(stream, info, ctx, curr_pos, 0);

        for (i = 0; i < nelmts; i++, ctx->cur_elmt++, elmt_counter++) {
            if (region_output && H5Tequal(type, H5T_STD_REF_DSETREG)) {
                char ref_name[1024];

                /* region data */
                region_id = H5Rdereference(container, H5R_DATASET_REGION, mem
                        + i * size);
                if (region_id >= 0) {
                    region_space = H5Rget_region(container, H5R_DATASET_REGION,
                            mem + i * size);
                    if (region_space >= 0) {
                        if (h5tools_is_zero(mem + i * size, H5Tget_size(type))) {
                            h5tools_str_append(&buffer, "NULL");
                        }
                        else {
                            H5Rget_name(region_id, H5R_DATASET_REGION, mem + i
                                    * size, (char*) ref_name, 1024);

                            /* Render the element */
                            h5tools_str_reset(&buffer);

                            h5tools_str_append(&buffer, info->dset_format,
                                    ref_name);

                            curr_pos = h5tools_render_element(stream, info,
                                    ctx, &buffer, curr_pos, flags, ncols,
                                    elmt_counter, i);

                            /* Print block information */
                            curr_pos = h5tools_dump_region_data_blocks(
                                    region_space, region_id, stream, info, ctx,
                                    &buffer, curr_pos, flags, ncols,
                                    elmt_counter, i);
                            /* Print point information */
                            curr_pos = h5tools_dump_region_data_points(
                                    region_space, region_id, stream, info, ctx,
                                    &buffer, curr_pos, flags, ncols,
                                    elmt_counter, i);

                        }
                        H5Sclose(region_space);
                    }
                    H5Dclose(region_id);
                }
                ctx->need_prefix = TRUE;
            } /* end if (region_output... */
            else {
                /* Render the element */
                h5tools_str_reset(&buffer);
                h5tools_str_sprint(&buffer, info, container, type, mem + i
                        * size, ctx);

                if (i + 1 < nelmts || (flags & END_OF_DATA) == 0)
                    h5tools_str_append(&buffer, "%s", OPT(info->elmt_suf1, ","));

//                curr_pos = h5tools_render_element(stream, info, ctx, &buffer,
//                        curr_pos, flags, ncols, elmt_counter, i);
//
//                /*
//                 * We need to break after each row of a dimension---> we should
//                 * break at the end of the each last dimension well that is the
//                 * way the dumper did it before
//                 */
//                if (info->arr_linebreak && ctx->cur_elmt) {
//                    if (elmt_counter == ctx->size_last_dim) {
//                        elmt_counter = 0;
//                    }
//                }
//

                s = h5tools_str_fmt(&buffer, 0, "%s");

                /*
                 * If the element would split on multiple lines if printed at our
                 * current location...
                 */
                if (info->line_multi_new == 1 &&
                        (ctx->cur_column + h5tools_ncols(s) +
                         strlen(OPT(info->elmt_suf2, " ")) +
                         strlen(OPT(info->line_suf, ""))) > ncols) {
                    if (ctx->prev_multiline) {
                        /*
                         * ... and the previous element also occupied more than one
                         * line, then start this element at the beginning of a line.
                         */
                        ctx->need_prefix = TRUE;
                    } else if ((ctx->prev_prefix_len + h5tools_ncols(s) +
                            strlen(OPT(info->elmt_suf2, " ")) +
                            strlen(OPT(info->line_suf, ""))) <= ncols) {
                        /*
                         * ...but *could* fit on one line otherwise, then we
                         * should end the current line and start this element on its
                         * own line.
                         */
                        ctx->need_prefix = TRUE;
                    }
                }

                /*
                 * We need to break after each row of a dimension---> we should
                 * break at the end of the each last dimension well that is the
                 * way the dumper did it before
                 */
                if (info->arr_linebreak && ctx->cur_elmt) {
                    if (ctx->size_last_dim && (ctx->cur_elmt % ctx->size_last_dim) == 0)
                        ctx->need_prefix = TRUE;

                    if ((hsize_t)elmt_counter == ctx->size_last_dim) {
                        ctx->need_prefix = TRUE;
                        elmt_counter = 0;
                    }
                }

                /*
                 * If the previous element occupied multiple lines and this element
                 * is too long to fit on a line then start this element at the
                 * beginning of the line.
                 */
                if (info->line_multi_new == 1 && ctx->prev_multiline &&
                        (ctx->cur_column + h5tools_ncols(s) +
                         strlen(OPT(info->elmt_suf2, " ")) +
                         strlen(OPT(info->line_suf, ""))) > ncols)
                    ctx->need_prefix = TRUE;

                /*
                 * If too many elements have already been printed then we need to
                 * start a new line.
                 */
                if (info->line_per_line > 0 && ctx->cur_elmt >= info->line_per_line)
                    ctx->need_prefix = TRUE;

                /*
                 * Each OPTIONAL_LINE_BREAK embedded in the rendered string can cause
                 * the data to split across multiple lines.  We display the sections
                 * one-at a time.
                 */
                for (secnum = 0, multiline = 0;
                         (section = strtok(secnum ? NULL : s, OPTIONAL_LINE_BREAK));
                         secnum++) {
                    /*
                     * If the current section plus possible suffix and end-of-line
                     * information would cause the output to wrap then we need to
                     * start a new line.
                     */

                    /*
                     * Added the info->skip_first because the dumper does not want
                     * this check to happen for the first line
                     */
                    if ((!info->skip_first || i) &&
                            (ctx->cur_column + strlen(section) +
                             strlen(OPT(info->elmt_suf2, " ")) +
                             strlen(OPT(info->line_suf, ""))) > ncols)
                        ctx->need_prefix = 1;

                    /*
                     * Print the prefix or separate the beginning of this element
                     * from the previous element.
                     */
                    if (ctx->need_prefix) {
                        if (secnum)
                            multiline++;

                        /* pass to the prefix in h5tools_simple_prefix the total
                         * position instead of the current stripmine position i;
                         * this is necessary to print the array indices
                         */
                        curr_pos = ctx->sm_pos + i;

                        h5tools_simple_prefix(stream, info, ctx, curr_pos, secnum);
                    } else if ((i || ctx->continuation) && secnum == 0) {
                        fputs(OPT(info->elmt_suf2, " "), stream);
                        ctx->cur_column += strlen(OPT(info->elmt_suf2, " "));
                    }

                    /* Print the section */
                    fputs(section, stream);
                    ctx->cur_column += strlen(section);
                }
            }

            ctx->prev_multiline = multiline;
        }

        h5tools_str_close(&buffer);
    }/* else bin */
}

hsize_t h5tools_render_element(FILE *stream, const h5tool_format_t *info,
        h5tools_context_t *ctx/*in,out*/,
        h5tools_str_t *buffer/*string into which to render */,
        hsize_t curr_pos/*total data element position*/, unsigned flags,
        size_t ncols, hsize_t elmt_counter, hsize_t i_count/*element counter*/) {
    char *s;
    char *section; /*a section of output  */
    int   secnum; /*section sequence number */
    int   multiline; /*datum was multiline  */

    s = h5tools_str_fmt(buffer, 0, "%s");

    /*
     * If the element would split on multiple lines if printed at our
     * current location...
     */
    if (info->line_multi_new == 1 && 
            (ctx->cur_column + h5tools_ncols(s) + 
            strlen(OPT(info->elmt_suf2, " ")) + 
            strlen(OPT(info->line_suf, ""))) > ncols) {
        if (ctx->prev_multiline) {
            /*
             * ... and the previous element also occupied more than one
             * line, then start this element at the beginning of a line.
             */
            ctx->need_prefix = TRUE;
        }
        else if ((ctx->prev_prefix_len + h5tools_ncols(s) + 
                strlen(OPT(info->elmt_suf2, " ")) + 
                strlen(OPT(info->line_suf, ""))) <= ncols) {
            /*
             * ...but *could* fit on one line otherwise, then we
             * should end the current line and start this element on its
             * own line.
             */
            ctx->need_prefix = TRUE;
        }
    }

    /*
     * We need to break after each row of a dimension---> we should
     * break at the end of the each last dimension well that is the
     * way the dumper did it before
     */
    if (info->arr_linebreak && ctx->cur_elmt) {
        if (ctx->size_last_dim && (ctx->cur_elmt % ctx->size_last_dim) == 0)
            ctx->need_prefix = TRUE;

        if (elmt_counter == ctx->size_last_dim) {
            ctx->need_prefix = TRUE;
        }
    }

    /*
     * If the previous element occupied multiple lines and this element
     * is too long to fit on a line then start this element at the
     * beginning of the line.
     */
    if (info->line_multi_new == 1 && 
            ctx->prev_multiline && 
            (ctx->cur_column + 
            h5tools_ncols(s) + 
            strlen(OPT(info->elmt_suf2, " ")) + 
            strlen(OPT(info->line_suf, ""))) > ncols)
        ctx->need_prefix = TRUE;

    /*
     * If too many elements have already been printed then we need to
     * start a new line.
     */
    if (info->line_per_line > 0 && ctx->cur_elmt >= info->line_per_line)
        ctx->need_prefix = TRUE;

    /*
     * Each OPTIONAL_LINE_BREAK embedded in the rendered string can cause
     * the data to split across multiple lines.  We display the sections
     * one-at a time.
     */
    for (secnum = 0, multiline = 0; (section = strtok(secnum ? NULL : s,
            OPTIONAL_LINE_BREAK)); secnum++) {
        /*
         * If the current section plus possible suffix and end-of-line
         * information would cause the output to wrap then we need to
         * start a new line.
         */

        /*
         * Added the info->skip_first because the dumper does not want
         * this check to happen for the first line
         */
        if ((!info->skip_first || i_count) && 
                (ctx->cur_column + 
                strlen(section) + 
                strlen(OPT(info->elmt_suf2, " ")) + 
                strlen(OPT(info->line_suf, ""))) > ncols)
            ctx->need_prefix = 1;

        /*
         * Print the prefix or separate the beginning of this element
         * from the previous element.
         */
        if (ctx->need_prefix) {
            if (secnum)
                multiline++;

            /* pass to the prefix in h5tools_simple_prefix the total
             * position instead of the current stripmine position i;
             * this is necessary to print the array indices
             */
            curr_pos = ctx->sm_pos + i_count;

            h5tools_simple_prefix(stream, info, ctx, curr_pos, secnum);
        }
        else if ((i_count || ctx->continuation) && secnum == 0) {
            fputs(OPT(info->elmt_suf2, " "), stream);
            ctx->cur_column += strlen(OPT(info->elmt_suf2, " "));
        }

        /* Print the section */
        fputs(section, stream);
        ctx->cur_column += strlen(section);
    }
    return curr_pos;
}

hsize_t h5tools_dump_region_data_blocks(hid_t region_space, hid_t region_id,
        FILE *stream, const h5tool_format_t *info,
        h5tools_context_t *ctx/*in,out*/,
        h5tools_str_t *buffer/*string into which to render */,
        hsize_t curr_pos/*total data element position*/, unsigned flags,
        size_t ncols, hsize_t elmt_counter, hsize_t i_count/*element counter*/) {
    hsize_t alloc_size;
    hsize_t *ptdata;
    hsize_t *dims1;
    hsize_t *start;
    hsize_t *count;
    hssize_t nblocks;
    size_t numelem;
    int ndims;
    int jndx;
    int i;
    int type_size;
    int blkndx;
    hid_t sid1;
    hid_t mem_space;
    hid_t dtype;
    hid_t type_id;
    void *region_buf;
    herr_t status;

    /*
     * This function fails if the region does not have blocks. 
     */
    H5E_BEGIN_TRY
        {
            nblocks = H5Sget_select_hyper_nblocks(region_space);
        }H5E_END_TRY;

    if (nblocks <= 0)
        return curr_pos;

    /* Print block information */
    ndims = H5Sget_simple_extent_ndims(region_space);
    /* Render the datatype element */
    h5tools_str_reset(buffer);

    h5tools_str_append(buffer, "{");
    curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
            flags, ncols, elmt_counter, i_count);

    /* Render the datatype element */
    h5tools_str_reset(buffer);

    ctx->indent_level++;
    ctx->need_prefix = TRUE;
    h5tools_str_append(buffer, "REGION_TYPE BLOCK  ");
    
    alloc_size = nblocks * ndims * 2 * sizeof(ptdata[0]);
    assert(alloc_size == (hsize_t) ((size_t) alloc_size)); /*check for overflow*/
    ptdata = (hsize_t*) malloc((size_t) alloc_size);
    H5_CHECK_OVERFLOW(nblocks, hssize_t, hsize_t);
    H5Sget_select_hyper_blocklist(region_space, (hsize_t) 0, (hsize_t) nblocks,
            ptdata);

    for (i = 0; i < nblocks; i++) {
        int j;

        h5tools_str_append(buffer, info->dset_blockformat_pre,
                i ? "," OPTIONAL_LINE_BREAK " " : "", (unsigned long) i);

        /* Start coordinates and opposite corner */
        for (j = 0; j < ndims; j++)
            h5tools_str_append(buffer, "%s%lu", j ? "," : "(",
                    (unsigned long) ptdata[i * 2 * ndims + j]);

        for (j = 0; j < ndims; j++)
            h5tools_str_append(buffer, "%s%lu", j ? "," : ")-(",
                    (unsigned long) ptdata[i * 2 * ndims + j + ndims]);

        h5tools_str_append(buffer, ")");
    }

    curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
            flags, ncols, elmt_counter, i_count);

    ctx->need_prefix = TRUE;

    dtype = H5Dget_type(region_id);
    type_id = H5Tget_native_type(dtype, H5T_DIR_DEFAULT);

    /* Render the datatype element */
    h5tools_str_reset(buffer);

    ctx->need_prefix = TRUE;
    h5tools_str_append(buffer, "%s %s ",
            h5tools_dump_header_format->datatypebegin,
            h5tools_dump_header_format->datatypeblockbegin);

    h5tools_print_datatype(buffer, info, ctx, type_id);

    if (HDstrlen(h5tools_dump_header_format->datatypeblockend)) {
        h5tools_str_append(buffer, "%s",
                h5tools_dump_header_format->datatypeblockend);
        if (HDstrlen(h5tools_dump_header_format->datatypeend))
            h5tools_str_append(buffer, " ");
    }
    if (HDstrlen(h5tools_dump_header_format->datatypeend))
        h5tools_str_append(buffer, "%s",
                h5tools_dump_header_format->datatypeend);

    curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
            flags, ncols, elmt_counter, i_count);

    ctx->need_prefix = TRUE;

    /* Render the datatype element */
    h5tools_str_reset(buffer);

    h5tools_str_append(buffer, "%s %s ",
            h5tools_dump_header_format->databegin,
            h5tools_dump_header_format->datablockbegin);

    curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
            flags, ncols, elmt_counter, i_count);

    ctx->need_prefix = TRUE;


    /* Get the dataspace of the dataset */
    sid1 = H5Dget_space(region_id);

    /* Allocate space for the dimension array */
    dims1 = (hsize_t *) malloc(sizeof(hsize_t) * ndims);

    /* find the dimensions of each data space from the block coordinates */
    numelem = 1;
    for (jndx = 0; jndx < ndims; jndx++) {
        dims1[jndx] = ptdata[jndx + ndims] - ptdata[jndx] + 1;
        numelem = dims1[jndx] * numelem;
    }

    /* Create dataspace for reading buffer */
    mem_space = H5Screate_simple(ndims, dims1, NULL);

    type_size = H5Tget_size(type_id);
    region_buf = malloc(type_size * numelem);

    /* Select (x , x , ..., x ) x (y , y , ..., y ) hyperslab for reading memory dataset */
    /*          1   2        n      1   2        n                                       */

    start = (hsize_t *) malloc(sizeof(hsize_t) * ndims);
    count = (hsize_t *) malloc(sizeof(hsize_t) * ndims);
    for (blkndx = 0; blkndx < nblocks; blkndx++) {
        if (blkndx > 0)
            ctx->need_prefix = TRUE;
        for (jndx = 0; jndx < ndims; jndx++) {
            start[jndx] = ptdata[jndx + blkndx * ndims * 2];
            count[jndx] = dims1[jndx];
        }

        status = H5Sselect_hyperslab(sid1, H5S_SELECT_SET, start, NULL,count,
                NULL);

        status = H5Dread(region_id, type_id, mem_space, sid1, H5P_DEFAULT,
                region_buf);

        /* Render the element */
        h5tools_str_reset(buffer);

        ctx->indent_level++;

        for (jndx = 0; jndx < numelem; jndx++) {
            h5tools_str_sprint(buffer, info, region_id, type_id, (region_buf
                    + jndx * type_size), ctx);

            if (jndx + 1 < numelem || (flags & END_OF_DATA) == 0)
                h5tools_str_append(buffer, "%s", OPT(info->elmt_suf1, ","));
        }
        curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
                flags, ncols, elmt_counter, i_count);

        ctx->indent_level--;
    }

    free(start);
    free(count);
    free(region_buf);
    free(ptdata);
    free(dims1);
    status = H5Tclose(dtype);
    status = H5Sclose(mem_space);
    status = H5Sclose(sid1);

    ctx->need_prefix = TRUE;

    /* Render the element */
    h5tools_str_reset(buffer);
    h5tools_str_append(buffer, "%s %s ",
            h5tools_dump_header_format->dataend,
            h5tools_dump_header_format->datablockend);
    curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
            flags, ncols, elmt_counter, i_count);

    ctx->indent_level--;
    ctx->need_prefix = TRUE;

    /* Render the element */
    h5tools_str_reset(buffer);
    h5tools_str_append(buffer, "}");
    curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
            flags, ncols, elmt_counter, i_count);

    return curr_pos;
}

hsize_t h5tools_dump_region_data_points(hid_t region_space, hid_t region_id,
        FILE *stream, const h5tool_format_t *info,
        h5tools_context_t *ctx/*in,out*/,
        h5tools_str_t *buffer/*string into which to render */,
        hsize_t curr_pos/*total data element position*/, unsigned flags,
        size_t ncols, hsize_t elmt_counter, hsize_t i_count/*element counter*/) {
    hssize_t npoints;
    hsize_t alloc_size;
    hsize_t *ptdata;
    hsize_t *dims1;
    int ndims;
    int jndx;
    int type_size;
    hid_t mem_space;
    hid_t dtype;
    hid_t type_id;
    void *region_buf;
    herr_t status;

    /*
     * This function fails if the region does not have blocks. 
     */
    H5E_BEGIN_TRY
        {
            npoints = H5Sget_select_elem_npoints(region_space);
        }H5E_END_TRY;
    if (npoints > 0) {
        int i;

        /* Render the datatype element */
        h5tools_str_reset(buffer);

        h5tools_str_append(buffer, "{");
        curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
                flags, ncols, elmt_counter, i_count);

        /* Render the datatype element */
        h5tools_str_reset(buffer);

        ctx->indent_level++;
        ctx->need_prefix = TRUE;
        h5tools_str_append(buffer, "REGION_TYPE POINT  ");

        /* Allocate space for the dimension array */
        ndims = H5Sget_simple_extent_ndims(region_space);
        alloc_size = npoints * ndims * sizeof(ptdata[0]);
        assert(alloc_size == (hsize_t) ((size_t) alloc_size)); /*check for overflow*/
        ptdata = malloc((size_t) alloc_size);
        H5_CHECK_OVERFLOW(npoints, hssize_t, hsize_t);
        H5Sget_select_elem_pointlist(region_space, (hsize_t) 0, (hsize_t) npoints,
                ptdata);

        for (i = 0; i < npoints; i++) {
            int j;

            h5tools_str_append(buffer, info->dset_ptformat_pre,
                    i ? "," OPTIONAL_LINE_BREAK " " : "", (unsigned long) i);

            for (j = 0; j < ndims; j++)
                h5tools_str_append(buffer, "%s%lu", j ? "," : "(",
                        (unsigned long) (ptdata[i * ndims + j]));

            h5tools_str_append(buffer, ")");
        }

        free(ptdata);

        curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
                flags, ncols, elmt_counter, i_count);

        ctx->need_prefix = TRUE;

        dtype = H5Dget_type(region_id);
        type_id = H5Tget_native_type(dtype, H5T_DIR_DEFAULT);

        /* Render the datatype element */
        h5tools_str_reset(buffer);
        h5tools_str_append(buffer, "%s %s ",
                h5tools_dump_header_format->datatypebegin,
                h5tools_dump_header_format->datatypeblockbegin);

        h5tools_print_datatype(buffer, info, ctx, type_id);

        if (HDstrlen(h5tools_dump_header_format->datatypeblockend)) {
            h5tools_str_append(buffer, "%s",
                    h5tools_dump_header_format->datatypeblockend);
            if (HDstrlen(h5tools_dump_header_format->datatypeend))
                h5tools_str_append(buffer, " ");
        }
        if (HDstrlen(h5tools_dump_header_format->datatypeend))
            h5tools_str_append(buffer, "%s",
                    h5tools_dump_header_format->datatypeend);

        curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
                flags, ncols, elmt_counter, i_count);

        ctx->need_prefix = TRUE;

        /* Render the datatype element */
        h5tools_str_reset(buffer);

        h5tools_str_append(buffer, "%s %s ",
                h5tools_dump_header_format->databegin,
                h5tools_dump_header_format->datablockbegin);

        curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
                flags, ncols, elmt_counter, i_count);

        ctx->need_prefix = TRUE;

        type_size = H5Tget_size(type_id);

        region_buf = malloc(type_size * npoints);

        /* Allocate space for the dimension array */
        dims1 = (hsize_t *) malloc(sizeof(hsize_t) * ndims);

        dims1[0] = npoints;
        mem_space = H5Screate_simple(1, dims1, NULL);

        status = H5Dread(region_id, type_id, mem_space, region_space,
                H5P_DEFAULT, region_buf);

        /* Render the element */
        h5tools_str_reset(buffer);

        ctx->indent_level++;

        for (jndx = 0; jndx < npoints; jndx++) {
            h5tools_str_sprint(buffer, info, region_id, type_id, (region_buf
                    + jndx * type_size), ctx);

            if (jndx + 1 < npoints || (flags & END_OF_DATA) == 0)
                h5tools_str_append(buffer, "%s", OPT(info->elmt_suf1, ","));
        }
        curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
                flags, ncols, elmt_counter, i_count);

        ctx->indent_level--;

        free(region_buf);
        free(dims1);
        status = H5Sclose(mem_space);
        status = H5Tclose(dtype);

        ctx->need_prefix = TRUE;

        /* Render the element */
        h5tools_str_reset(buffer);
        h5tools_str_append(buffer, "%s %s ",
                h5tools_dump_header_format->dataend,
                h5tools_dump_header_format->datablockend);
        curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
                flags, ncols, elmt_counter, i_count);

        ctx->indent_level--;
        ctx->need_prefix = TRUE;

        /* Render the element */
        h5tools_str_reset(buffer);
        h5tools_str_append(buffer, "}");
        curr_pos = h5tools_render_element(stream, info, ctx, buffer, curr_pos,
                flags, ncols, elmt_counter, i_count);
    }
    return curr_pos;
}

/*-------------------------------------------------------------------------
 * Audience:    Public
 * Chapter:     H5Tools Library
 * Purpose:     Dump out a subset of a dataset.
 * Description:
 *
 *  Select a hyperslab from the dataset DSET using the parameters
 *   specified in SSET. Dump this out to STREAM.
 *
 *  Hyperslabs select "count" blocks of size "block", spaced "stride" elements
 *   from each other, starting at coordinate "start".
 *
 * Return:
 *      On success, return SUCCEED. Otherwise, the function returns FAIL.
 *
 * Original programmer:
 *      Bill Wendling, Wednesday, March 07, 2001
 *
 * Rewritten with modified algorithm by:
 *      Pedro Vicente, Wednesday, January 16, 2008, contributions from Quincey Koziol
 *
 * Algorithm
 *
 * In a inner loop, the parameters from SSET are translated into temporary
 * variables so that 1 row is printed at a time (getting the coordinate indices
 * at each row).
 * We define the stride, count and block to be 1 in the row dimension to achieve
 * this and advance until all points are printed.
 * An outer loop for cases where dimensionality is greater than 2D is made.
 * In each iteration, the 2D block is displayed in the inner loop. The remaining
 * slower dimensions above the first 2 are incremented one at a time in the outer loop
 *
 * The element position is obtained from the matrix according to:
 *       Given an index I(z,y,x) its position from the beginning of an array
 *       of sizes A(size_z, size_y,size_x) is given by
 *       Position of I(z,y,x) = index_z * size_y * size_x
 *                             + index_y * size_x
 *                             + index_x
 *
 *-------------------------------------------------------------------------
 */
static herr_t h5tools_dump_simple_subset(FILE *stream,
        const h5tool_format_t *info, hid_t dset, hid_t p_type,
        struct subset_t *sset, int indentlevel) {
    herr_t ret; /* the value to return */
    hid_t f_space; /* file data space */
    size_t i; /* counters  */
    size_t j; /* counters  */
    hsize_t n; /* counters  */
    hsize_t zero = 0; /* vector of zeros */
    unsigned int flags; /* buffer extent flags */
    hsize_t total_size[H5S_MAX_RANK];/* total size of dataset*/
    hsize_t elmtno; /* elemnt index  */
    hsize_t low[H5S_MAX_RANK]; /* low bound of hyperslab */
    hsize_t high[H5S_MAX_RANK]; /* higher bound of hyperslab */
    h5tools_context_t ctx; /* print context  */
    size_t p_type_nbytes; /* size of memory type */
    hsize_t sm_size[H5S_MAX_RANK]; /* stripmine size */
    hsize_t sm_nbytes; /* bytes per stripmine */
    hsize_t sm_nelmts; /* elements per stripmine*/
    unsigned char *sm_buf = NULL; /* buffer for raw data */
    hid_t sm_space; /* stripmine data space */
    hsize_t count; /* hyperslab count */
    hsize_t outer_count; /* offset count */
    unsigned int row_dim; /* index of row_counter dimension */
    int current_outer_dim; /* dimension for start */
    hsize_t temp_start[H5S_MAX_RANK];/* temporary start inside offset count loop */
    hsize_t max_start[H5S_MAX_RANK]; /* maximum start inside offset count loop */
    hsize_t temp_count[H5S_MAX_RANK];/* temporary count inside offset count loop  */
    hsize_t temp_block[H5S_MAX_RANK];/* temporary block size used in loop  */
    hsize_t temp_stride[H5S_MAX_RANK];/* temporary stride size used in loop  */
    int reset_dim;
    hsize_t size_row_block; /* size for blocks along rows */

#if defined (SANITY_CHECK)
    hsize_t total_points = 1; /* to print */
    hsize_t printed_points = 0; /* printed */
#endif

    ret = FAIL; /* be pessimistic */
    f_space = H5Dget_space(dset);

    if (f_space == FAIL)
        goto done;

    /*
     * check that everything looks okay. the dimensionality must not be too
     * great and the dimensionality of the items selected for printing must
     * match the dimensionality of the dataset.
     */
    memset(&ctx, 0, sizeof(ctx));
    ctx.indent_level = indentlevel;
    ctx.need_prefix = 1;
    ctx.ndims = H5Sget_simple_extent_ndims(f_space);

    if ((size_t) ctx.ndims > NELMTS(sm_size))
        goto done_close;

    /* assume entire data space to be printed */
    if (ctx.ndims > 0)
        for (i = 0; i < (size_t) ctx.ndims; i++)
            ctx.p_min_idx[i] = 0;

    H5Sget_simple_extent_dims(f_space, total_size, NULL);
    ctx.size_last_dim = total_size[ctx.ndims - 1];

    if (ctx.ndims == 1)
        row_dim = 0;
    else
        row_dim = ctx.ndims - 2;

    /* get the offset count */
    outer_count = 1;
    if (ctx.ndims > 2)
        for (i = 0; i < (size_t) ctx.ndims - 2; i++) {
            /* consider block size */
            outer_count = outer_count * sset->count[i] * sset->block[i];

        }

    if (ctx.ndims > 0)
        init_acc_pos(&ctx, total_size);

    /* calculate total number of points to print */
#if defined (SANITY_CHECK)
    for (i = 0; i < (size_t) ctx.ndims; i++) {
        total_points *= sset->count[i] * sset->block[i];;
    }
#endif

    /* initialize temporary start, count and maximum start */
    for (i = 0; i < (size_t) ctx.ndims; i++) {
        temp_start[i] = sset->start[i];
        temp_count[i] = sset->count[i];
        temp_block[i] = sset->block[i];
        temp_stride[i] = sset->stride[i];
        max_start[i] = 0;

    }
    if (ctx.ndims > 2) {
        for (i = 0; i < (size_t) ctx.ndims - 2; i++) {
            max_start[i] = temp_start[i] + sset->count[i];
            temp_count[i] = 1;

        }
    }

    /* offset loop */
    for (n = 0; n < outer_count; n++) {

        hsize_t row_counter = 0;

        /* number of read iterations in inner loop, read by rows, to match 2D display */
        if (ctx.ndims > 1) {

            /* count is the number of iterations to display all the rows,
             the block size count times */
            count = sset->count[row_dim] * sset->block[row_dim];

            /* always 1 row_counter at a time, that is a block of size 1, 1 time */
            temp_count[row_dim] = 1;
            temp_block[row_dim] = 1;

            /* advance 1 row_counter at a time  */
            if (sset->block[row_dim] > 1)
                temp_stride[row_dim] = 1;

        }
        /* for the 1D case */
        else {
            count = 1;
        }

        size_row_block = sset->block[row_dim];

        /* display loop */
        for (; count > 0; temp_start[row_dim] += temp_stride[row_dim], count--) {

            /* jump rows if size of block exceeded
             cases where block > 1 only and stride > block */
            if (size_row_block > 1 && row_counter == size_row_block
                    && sset->stride[row_dim] > sset->block[row_dim]) {

                hsize_t increase_rows = sset->stride[row_dim]
                        - sset->block[row_dim];

                temp_start[row_dim] += increase_rows;

                row_counter = 0;

            }

            row_counter++;

            /* calculate the potential number of elements we're going to print */
            H5Sselect_hyperslab(f_space, H5S_SELECT_SET, temp_start,
                    temp_stride, temp_count, temp_block);
            sm_nelmts = H5Sget_select_npoints(f_space);

            if (sm_nelmts == 0) {
                /* nothing to print */
                ret = SUCCEED;
                goto done_close;
            }

            /*
             * determine the strip mine size and allocate a buffer. the strip mine is
             * a hyperslab whose size is manageable.
             */
            sm_nbytes = p_type_nbytes = H5Tget_size(p_type);

            if (ctx.ndims > 0)
                for (i = ctx.ndims; i > 0; --i) 
                {
                    hsize_t size = H5TOOLS_BUFSIZE / sm_nbytes;
                    if ( size == 0) /* datum size > H5TOOLS_BUFSIZE */
                        size = 1;
                    sm_size[i - 1] = MIN(total_size[i - 1], size);
                    sm_nbytes *= sm_size[i - 1];
                    assert(sm_nbytes > 0);
                }

            assert(sm_nbytes == (hsize_t) ((size_t) sm_nbytes)); /*check for overflow*/
            sm_buf = malloc((size_t) sm_nelmts * p_type_nbytes);
            sm_space = H5Screate_simple(1, &sm_nelmts, NULL);

            H5Sselect_hyperslab(sm_space, H5S_SELECT_SET, &zero, NULL,
                    &sm_nelmts, NULL);

            /* read the data */
            if (H5Dread(dset, p_type, sm_space, f_space, H5P_DEFAULT, sm_buf)
                    < 0) {
                H5Sclose(f_space);
                H5Sclose(sm_space);
                free(sm_buf);
                return FAIL;
            }

            /* print the data */
            flags = START_OF_DATA;

            if (count == 1)
                flags |= END_OF_DATA;

            for (i = 0; i < ctx.ndims; i++)
                ctx.p_max_idx[i] = ctx.p_min_idx[i] + MIN(total_size[i],
                        sm_size[i]);

            /* print array indices. get the lower bound of the hyperslab and calulate
             the element position at the start of hyperslab */
            H5Sget_select_bounds(f_space, low, high);
            elmtno = 0;
            for (i = 0; i < (size_t) ctx.ndims - 1; i++) {
                hsize_t offset = 1; /* accumulation of the previous dimensions */
                for (j = i + 1; j < (size_t) ctx.ndims; j++)
                    offset *= total_size[j];
                elmtno += low[i] * offset;
            }
            elmtno += low[ctx.ndims - 1];

            /* initialize the current stripmine position; this is necessary to print the array
             indices */
            ctx.sm_pos = elmtno;

            h5tools_dump_simple_data(stream, info, dset, &ctx, flags,
                    sm_nelmts, p_type, sm_buf);
            free(sm_buf);

            /* we need to jump to next line and update the index */
            ctx.need_prefix = 1;

            ctx.continuation++;

#if defined (SANITY_CHECK)
            printed_points += sm_nelmts;
#endif

        } /* count */

        if (ctx.ndims > 2) {
            /* dimension for start */
            current_outer_dim = (ctx.ndims - 2) - 1;

            /* set start to original from current_outer_dim up */
            for (i = current_outer_dim + 1; i < ctx.ndims; i++) {
                temp_start[i] = sset->start[i];
            }

            /* increment start dimension */
            do {
                reset_dim = 0;
                temp_start[current_outer_dim]++;
                if (temp_start[current_outer_dim]
                        >= max_start[current_outer_dim]) {
                    temp_start[current_outer_dim]
                            = sset->start[current_outer_dim];

                    /* consider block */
                    if (sset->block[current_outer_dim] > 1)
                        temp_start[current_outer_dim]++;

                    current_outer_dim--;
                    reset_dim = 1;
                }
            } while (current_outer_dim >= 0 && reset_dim);

        } /* ctx.ndims > 1 */

    } /* outer_count */

#if defined (SANITY_CHECK)
    assert(printed_points == total_points);
#endif

    /* Terminate the output */
    if (ctx.cur_column) {
        fputs(OPT(info->line_suf, ""), stream);
        putc('\n', stream);
        fputs(OPT(info->line_sep, ""), stream);
    }

    ret = SUCCEED;

    done_close: H5Sclose(f_space);
    done: return ret;
}

/*-------------------------------------------------------------------------
 * Audience:    Public
 * Chapter:     H5Tools Library
 * Purpose: Print some values from a dataset with a simple data space.
 * Description:
 *      This is a special case of h5tools_dump_dset(). This function only
 *      intended for dumping datasets -- it does strip mining and some other
 *      things which are unnecessary for smaller objects such as attributes
 *      (to print small objects like attributes simply read the attribute and
 *      call h5tools_dump_simple_mem()).
 * Return:
 *      On success, the function returns SUCCEED. Otherwise, the function
 *      returns FAIL.
 * Programmer:
 *      Robb Matzke, Thursday, July 23, 1998
 * Modifications:
 *-------------------------------------------------------------------------
 */
static int 
h5tools_dump_simple_dset(FILE *stream, const h5tool_format_t *info,
                         hid_t dset, hid_t p_type, int indentlevel) {
    hid_t               f_space;                  /* file data space */
    hsize_t             elmtno;                   /* counter  */
    size_t              i;                        /* counter  */
    int                 carry;                    /* counter carry value */
    hsize_t             zero[8];                  /* vector of zeros */
    unsigned int        flags;                    /* buffer extent flags */
    hsize_t             total_size[H5S_MAX_RANK]; /* total size of dataset*/

    /* Print info */
    h5tools_context_t   ctx;                      /* print context  */
    size_t              p_type_nbytes;            /* size of memory type */
    hsize_t             p_nelmts;                 /* total selected elmts */

    /* Stripmine info */
    hsize_t             sm_size[H5S_MAX_RANK];    /* stripmine size */
    hsize_t             sm_nbytes;                /* bytes per stripmine */
    hsize_t             sm_nelmts;                /* elements per stripmine*/
    unsigned char      *sm_buf = NULL;            /* buffer for raw data */
    hid_t               sm_space;                 /* stripmine data space */

    /* Hyperslab info */
    hsize_t             hs_offset[H5S_MAX_RANK];  /* starting offset */
    hsize_t             hs_size[H5S_MAX_RANK];    /* size this pass */
    hsize_t             hs_nelmts;                /* elements in request */

    /* VL data special information */
    unsigned int        vl_data = 0; /* contains VL datatypes */

    f_space = H5Dget_space(dset);

    if (f_space == FAIL)
        return FAIL;

    /*
     * Check that everything looks okay. The dimensionality must not be too
     * great and the dimensionality of the items selected for printing must
     * match the dimensionality of the dataset.
     */
    memset(&ctx, 0, sizeof(ctx));
    ctx.indent_level = indentlevel;
    ctx.need_prefix = 1;
    ctx.ndims = H5Sget_simple_extent_ndims(f_space);

    if ((size_t)ctx.ndims > NELMTS(sm_size)) {
        H5Sclose(f_space);
        return FAIL;
    }

    /* Assume entire data space to be printed */
    if (ctx.ndims > 0)
        for (i = 0; i < (size_t)ctx.ndims; i++)
            ctx.p_min_idx[i] = 0;

    H5Sget_simple_extent_dims(f_space, total_size, NULL);

    /* calculate the number of elements we're going to print */
    p_nelmts = 1;

    if (ctx.ndims > 0) {
        for (i = 0; i < ctx.ndims; i++)
            p_nelmts *= total_size[i];
        ctx.size_last_dim = (total_size[ctx.ndims - 1]);
    } /* end if */
    else
        ctx.size_last_dim = 0;

    if (p_nelmts == 0) {
        /* nothing to print */
        H5Sclose(f_space);
        return SUCCEED;
    }

    /* Check if we have VL data in the dataset's datatype */
    if (H5Tdetect_class(p_type, H5T_VLEN) == TRUE)
        vl_data = TRUE;

    /*
     * Determine the strip mine size and allocate a buffer. The strip mine is
     * a hyperslab whose size is manageable.
     */
    sm_nbytes = p_type_nbytes = H5Tget_size(p_type);

    if (ctx.ndims > 0) {
        for (i = ctx.ndims; i > 0; --i) {
            hsize_t size = H5TOOLS_BUFSIZE / sm_nbytes;
            if ( size == 0) /* datum size > H5TOOLS_BUFSIZE */
                size = 1;
            sm_size[i - 1] = MIN(total_size[i - 1], size);
            sm_nbytes *= sm_size[i - 1];
            assert(sm_nbytes > 0);
        }
    }

    assert(sm_nbytes == (hsize_t)((size_t)sm_nbytes)); /*check for overflow*/
    sm_buf = malloc((size_t)sm_nbytes);

    sm_nelmts = sm_nbytes / p_type_nbytes;
    sm_space = H5Screate_simple(1, &sm_nelmts, NULL);

    if (ctx.ndims > 0)
        init_acc_pos(&ctx, total_size);

    /* The stripmine loop */
    memset(hs_offset, 0, sizeof hs_offset);
    memset(zero, 0, sizeof zero);

    for (elmtno = 0; elmtno < p_nelmts; elmtno += hs_nelmts) {
        /* Calculate the hyperslab size */
        if (ctx.ndims > 0) {
            for (i = 0, hs_nelmts = 1; i < ctx.ndims; i++) {
                hs_size[i] = MIN(total_size[i] - hs_offset[i], sm_size[i]);
                ctx.p_max_idx[i] = ctx.p_min_idx[i] + hs_size[i];
                hs_nelmts *= hs_size[i];
            }

            H5Sselect_hyperslab(f_space, H5S_SELECT_SET, hs_offset, NULL,
                                hs_size, NULL);
            H5Sselect_hyperslab(sm_space, H5S_SELECT_SET, zero, NULL,
                                &hs_nelmts, NULL);
        }
        else {
            H5Sselect_all(f_space);
            H5Sselect_all(sm_space);
            hs_nelmts = 1;
        }

        /* Read the data */
        if (H5Dread(dset, p_type, sm_space, f_space, H5P_DEFAULT, sm_buf) < 0) {
            H5Sclose(f_space);
            H5Sclose(sm_space);
            free(sm_buf);
            return FAIL;
        }

        /* Print the data */
        flags = (elmtno == 0) ? START_OF_DATA : 0;
        flags |= ((elmtno + hs_nelmts) >= p_nelmts) ? END_OF_DATA : 0;

        /* initialize the current stripmine position; this is necessary to print the array
         indices */
        ctx.sm_pos = elmtno;

        h5tools_dump_simple_data(stream, info, dset, &ctx, flags, hs_nelmts,
                             p_type, sm_buf);

        /* Reclaim any VL memory, if necessary */
        if (vl_data)
            H5Dvlen_reclaim(p_type, sm_space, H5P_DEFAULT, sm_buf);

        /* Calculate the next hyperslab offset */
        for (i = ctx.ndims, carry = 1; i > 0 && carry; --i) {
            ctx.p_min_idx[i - 1] = ctx.p_max_idx[i - 1];
            hs_offset[i - 1] += hs_size[i - 1];

            if (hs_offset[i - 1] == total_size[i - 1])
                hs_offset[i - 1] = 0;
            else
                carry = 0;
        }

        ctx.continuation++;
    }

    /* Terminate the output */
    if (ctx.cur_column) {
        fputs(OPT(info->line_suf, ""), stream);
        putc('\n', stream);
        fputs(OPT(info->line_sep, ""), stream);
    }

    H5Sclose(sm_space);
    H5Sclose(f_space);

    free(sm_buf);

    return SUCCEED;
}

/*-------------------------------------------------------------------------
 * Function: h5tools_dump_simple_mem
 *
 * Purpose: Print some values from memory with a simple data space.
 *  This is a special case of h5tools_dump_mem().
 *
 * Return: Success:    SUCCEED
 *
 *  Failure:    FAIL
 *
 * Programmer: Robb Matzke
 *              Thursday, July 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int h5tools_dump_simple_mem(FILE *stream, const h5tool_format_t *info,
        hid_t obj_id, hid_t type, hid_t space, void *mem, int indentlevel) {
    int i; /*counters  */
    hsize_t nelmts; /*total selected elmts */
    h5tools_context_t ctx; /*printing context */

    /*
     * Check that everything looks okay.  The dimensionality must not be too
     * great and the dimensionality of the items selected for printing must
     * match the dimensionality of the dataset.
     */
    memset(&ctx, 0, sizeof(ctx));
    ctx.ndims = H5Sget_simple_extent_ndims(space);

    if ((size_t) ctx.ndims > NELMTS(ctx.p_min_idx))
        return FAIL;

    ctx.indent_level = indentlevel;
    ctx.need_prefix = 1;

    /* Assume entire data space to be printed */
    for (i = 0; i < ctx.ndims; i++)
        ctx.p_min_idx[i] = 0;

    H5Sget_simple_extent_dims(space, ctx.p_max_idx, NULL);

    for (i = 0, nelmts = 1; ctx.ndims != 0 && i < ctx.ndims; i++)
        nelmts *= ctx.p_max_idx[i] - ctx.p_min_idx[i];

    if (nelmts == 0)
        return SUCCEED; /*nothing to print*/
    if (ctx.ndims > 0) {
        assert(ctx.p_max_idx[ctx.ndims - 1]
                == (hsize_t) ((int) ctx.p_max_idx[ctx.ndims - 1]));
        ctx.size_last_dim = (int) (ctx.p_max_idx[ctx.ndims - 1]);
    } /* end if */
    else
        ctx.size_last_dim = 0;

    if (ctx.ndims > 0)
        init_acc_pos(&ctx, ctx.p_max_idx);

    /* Print it */
    h5tools_dump_simple_data(stream, info, obj_id, &ctx, START_OF_DATA
            | END_OF_DATA, nelmts, type, mem);

    /* Terminate the output */
    if (ctx.cur_column) {
        fputs(OPT(info->line_suf, ""), stream);
        putc('\n', stream);
        fputs(OPT(info->line_sep, ""), stream);
    }

    return SUCCEED;
}

/*-------------------------------------------------------------------------
 * Function: h5tools_dump_dset
 *
 * Purpose: Print some values from a dataset DSET to the file STREAM
 *  after converting all types to P_TYPE (which should be a
 *  native type).  If P_TYPE is a negative value then it will be
 *  computed from the dataset type using only native types.
 *
 * Note: This function is intended only for datasets since it does
 *  some things like strip mining which are unnecessary for
 *  smaller objects such as attributes. The easiest way to print
 *  small objects is to read the object into memory and call
 *  h5tools_dump_mem().
 *
 * Return: Success:    SUCCEED
 *
 *  Failure:    FAIL
 *
 * Programmer: Robb Matzke
 *              Thursday, July 23, 1998
 *
 * Modifications:
 *   Robb Matzke, 1999-06-07
 *  If info->raw is set then the memory datatype will be the same
 *  as the file datatype.
 *
 *  Bill Wendling, 2001-02-27
 *  Renamed to ``h5tools_dump_dset'' and added the subsetting
 *  parameter.
 *
 *-------------------------------------------------------------------------
 */
int 
h5tools_dump_dset(FILE *stream, const h5tool_format_t *info, hid_t dset,
                  hid_t _p_type, struct subset_t *sset, int indentlevel) {
    hid_t     f_space;
    hid_t     p_type = _p_type;
    hid_t     f_type;
    H5S_class_t space_type;
    int       status = FAIL;
    h5tool_format_t info_dflt;

    /* Use default values */
    if (!stream)
        stream = stdout;

    if (!info) {
        memset(&info_dflt, 0, sizeof info_dflt);
        info = &info_dflt;
    }

    if (p_type < 0) {
        f_type = H5Dget_type(dset);

        if (info->raw || bin_form == 1)
            p_type = H5Tcopy(f_type);
        else if (bin_form == 2)
            p_type = h5tools_get_little_endian_type(f_type);
        else if (bin_form == 3)
            p_type = h5tools_get_big_endian_type(f_type);
        else
            p_type = h5tools_get_native_type(f_type);

        H5Tclose(f_type);

        if (p_type < 0)
            goto done;
    }

    /* Check the data space */
    f_space = H5Dget_space(dset);

    space_type = H5Sget_simple_extent_type(f_space);

    /* Print the data */
    if (space_type == H5S_SIMPLE || space_type == H5S_SCALAR) {
        if (!sset) {
            status = h5tools_dump_simple_dset(rawdatastream, info, dset,
                    p_type, indentlevel);
        }
        else {
            status = h5tools_dump_simple_subset(rawdatastream, info, dset,
                    p_type, sset, indentlevel);
        }
    }
    else
        /* space is H5S_NULL */
        status = SUCCEED;

    /* Close the dataspace */
    H5Sclose(f_space);

    done: if (p_type != _p_type)
        H5Tclose(p_type);

    return status;
}

/*-------------------------------------------------------------------------
 * Function: h5tools_dump_mem
 *
 * Purpose: Displays the data contained in MEM. MEM must have the
 *  specified data TYPE and SPACE.  Currently only simple data
 *  spaces are allowed and only the `all' selection.
 *
 * Return: Success:    SUCCEED
 *
 *  Failure:    FAIL
 *
 * Programmer: Robb Matzke
 *              Wednesday, January 20, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int h5tools_dump_mem(FILE *stream, const h5tool_format_t *info, hid_t obj_id,
        hid_t type, hid_t space, void *mem, int indentlevel) {
    h5tool_format_t info_dflt;

    /* Use default values */
    if (!stream)
        stream = stdout;

    if (!info) {
        memset(&info_dflt, 0, sizeof(info_dflt));
        info = &info_dflt;
    }

    /* Check the data space */
    if (H5Sis_simple(space) <= 0)
        return -1;

    return h5tools_dump_simple_mem(stream, info, obj_id, type, space, mem,
            indentlevel);
}

/*-------------------------------------------------------------------------
 * Function:    print_datatype
 *
 * Purpose:     print the datatype.
 *
 * Return:      void
 *
 * Programmer:  Ruey-Hsia Li
 *
 * Modifications: pvn, March 28, 2006
 *  print information about type when a native match is not possible
 *
 *-------------------------------------------------------------------------
 */
void h5tools_print_datatype(h5tools_str_t *buffer/*in,out*/,
        const h5tool_format_t *info, h5tools_context_t *ctx/*in,out*/,
        hid_t type) {
    char *mname;
    hid_t mtype, str_type;
    unsigned nmembers;
    unsigned ndims;
    unsigned i;
    size_t size = 0;
    hsize_t dims[H5TOOLS_DUMP_MAX_RANK];
    H5T_str_t str_pad;
    H5T_cset_t cset;
    H5T_order_t order;
    hid_t super;
    hid_t tmp_type;
    htri_t is_vlstr = FALSE;
    const char *order_s = NULL; /* byte order string */
    H5T_sign_t sign; /* sign scheme value */
    const char *sign_s = NULL; /* sign scheme string */

    switch (H5Tget_class(type)) {
    case H5T_INTEGER:
        if (H5Tequal(type, H5T_STD_I8BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_I8BE");
        }
        else if (H5Tequal(type, H5T_STD_I8LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_I8LE");
        }
        else if (H5Tequal(type, H5T_STD_I16BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_I16BE");
        }
        else if (H5Tequal(type, H5T_STD_I16LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_I16LE");
        }
        else if (H5Tequal(type, H5T_STD_I32BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_I32BE");
        }
        else if (H5Tequal(type, H5T_STD_I32LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_I32LE");
        }
        else if (H5Tequal(type, H5T_STD_I64BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_I64BE");
        }
        else if (H5Tequal(type, H5T_STD_I64LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_I64LE");
        }
        else if (H5Tequal(type, H5T_STD_U8BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_U8BE");
        }
        else if (H5Tequal(type, H5T_STD_U8LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_U8LE");
        }
        else if (H5Tequal(type, H5T_STD_U16BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_U16BE");
        }
        else if (H5Tequal(type, H5T_STD_U16LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_U16LE");
        }
        else if (H5Tequal(type, H5T_STD_U32BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_U32BE");
        }
        else if (H5Tequal(type, H5T_STD_U32LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_U32LE");
        }
        else if (H5Tequal(type, H5T_STD_U64BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_U64BE");
        }
        else if (H5Tequal(type, H5T_STD_U64LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_U64LE");
        }
        else if (H5Tequal(type, H5T_NATIVE_SCHAR) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_SCHAR");
        }
        else if (H5Tequal(type, H5T_NATIVE_UCHAR) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_UCHAR");
        }
        else if (H5Tequal(type, H5T_NATIVE_SHORT) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_SHORT");
        }
        else if (H5Tequal(type, H5T_NATIVE_USHORT) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_USHORT");
        }
        else if (H5Tequal(type, H5T_NATIVE_INT) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_INT");
        }
        else if (H5Tequal(type, H5T_NATIVE_UINT) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_UINT");
        }
        else if (H5Tequal(type, H5T_NATIVE_LONG) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_LONG");
        }
        else if (H5Tequal(type, H5T_NATIVE_ULONG) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_ULONG");
        }
        else if (H5Tequal(type, H5T_NATIVE_LLONG) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_LLONG");
        }
        else if (H5Tequal(type, H5T_NATIVE_ULLONG) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_ULLONG");
        }
        else {

            /* byte order */
            if (H5Tget_size(type) > 1) {
                order = H5Tget_order(type);
                if (H5T_ORDER_LE == order) {
                    order_s = " little-endian";
                }
                else if (H5T_ORDER_BE == order) {
                    order_s = " big-endian";
                }
                else if (H5T_ORDER_VAX == order) {
                    order_s = " mixed-endian";
                }
                else {
                    order_s = " unknown-byte-order";
                }
            }
            else {
                order_s = "";
            }

            /* sign */
            if ((sign = H5Tget_sign(type)) >= 0) {
                if (H5T_SGN_NONE == sign) {
                    sign_s = " unsigned";
                }
                else if (H5T_SGN_2 == sign) {
                    sign_s = "";
                }
                else {
                    sign_s = " unknown-sign";
                }
            }
            else {
                sign_s = " unknown-sign";
            }

            /* print size, order, and sign  */
            h5tools_str_append(buffer, "%lu-bit%s%s integer",
                    (unsigned long) (8 * H5Tget_size(type)), order_s, sign_s);
        }
        break;

    case H5T_FLOAT:
        if (H5Tequal(type, H5T_IEEE_F32BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_IEEE_F32BE");
        }
        else if (H5Tequal(type, H5T_IEEE_F32LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_IEEE_F32LE");
        }
        else if (H5Tequal(type, H5T_IEEE_F64BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_IEEE_F64BE");
        }
        else if (H5Tequal(type, H5T_IEEE_F64LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_IEEE_F64LE");
        }
        else if (H5Tequal(type, H5T_VAX_F32) == TRUE) {
            h5tools_str_append(buffer, "H5T_VAX_F32");
        }
        else if (H5Tequal(type, H5T_VAX_F64) == TRUE) {
            h5tools_str_append(buffer, "H5T_VAX_F64");
        }
        else if (H5Tequal(type, H5T_NATIVE_FLOAT) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_FLOAT");
        }
        else if (H5Tequal(type, H5T_NATIVE_DOUBLE) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_DOUBLE");
#if H5_SIZEOF_LONG_DOUBLE !=0
        }
        else if (H5Tequal(type, H5T_NATIVE_LDOUBLE) == TRUE) {
            h5tools_str_append(buffer, "H5T_NATIVE_LDOUBLE");
#endif
        }
        else {

            /* byte order */
            if (H5Tget_size(type) > 1) {
                order = H5Tget_order(type);
                if (H5T_ORDER_LE == order) {
                    order_s = " little-endian";
                }
                else if (H5T_ORDER_BE == order) {
                    order_s = " big-endian";
                }
                else if (H5T_ORDER_VAX == order) {
                    order_s = " mixed-endian";
                }
                else {
                    order_s = " unknown-byte-order";
                }
            }
            else {
                order_s = "";
            }

            /* print size and byte order */
            h5tools_str_append(buffer, "%lu-bit%s floating-point",
                    (unsigned long) (8 * H5Tget_size(type)), order_s);

        }
        break;

    case H5T_TIME:
        h5tools_str_append(buffer, "H5T_TIME: not yet implemented");
        break;

    case H5T_STRING:
        /* Make a copy of type in memory in case when TYPE is on disk, the size
         * will be bigger than in memory.  This makes it easier to compare
         * types in memory. */
        tmp_type = H5Tcopy(type);
        size = H5Tget_size(tmp_type);
        str_pad = H5Tget_strpad(tmp_type);
        cset = H5Tget_cset(tmp_type);
        is_vlstr = H5Tis_variable_str(tmp_type);

        h5tools_str_append(buffer, "H5T_STRING %s\n",
                h5tools_dump_header_format->strblockbegin);
        ctx->indent_level++;

        if (is_vlstr)
            h5tools_str_append(buffer, "%s H5T_VARIABLE;\n", STRSIZE);
        else
            h5tools_str_append(buffer, "%s %d;\n", STRSIZE, (int) size);

        h5tools_str_append(buffer, "%s ", STRPAD);
        if (str_pad == H5T_STR_NULLTERM)
            h5tools_str_append(buffer, "H5T_STR_NULLTERM;\n");
        else if (str_pad == H5T_STR_NULLPAD)
            h5tools_str_append(buffer, "H5T_STR_NULLPAD;\n");
        else if (str_pad == H5T_STR_SPACEPAD)
            h5tools_str_append(buffer, "H5T_STR_SPACEPAD;\n");
        else
            h5tools_str_append(buffer, "H5T_STR_ERROR;\n");

        h5tools_str_append(buffer, "%s ", CSET);

        if (cset == H5T_CSET_ASCII)
            h5tools_str_append(buffer, "H5T_CSET_ASCII;\n");
        else
            h5tools_str_append(buffer, "unknown_cset;\n");

        str_type = H5Tcopy(H5T_C_S1);
        if (is_vlstr)
            H5Tset_size(str_type, H5T_VARIABLE);
        else
            H5Tset_size(str_type, size);
        H5Tset_cset(str_type, cset);
        H5Tset_strpad(str_type, str_pad);

        h5tools_str_append(buffer, "%s ", CTYPE);

        /* Check C variable-length string first. Are the two types equal? */
        if (H5Tequal(tmp_type, str_type)) {
            h5tools_str_append(buffer, "H5T_C_S1;\n");
            goto done;
        }

        /* Change the endianness and see if they're equal. */
        order = H5Tget_order(tmp_type);
        if (order == H5T_ORDER_LE)
            H5Tset_order(str_type, H5T_ORDER_LE);
        else if (order == H5T_ORDER_BE)
            H5Tset_order(str_type, H5T_ORDER_BE);

        if (H5Tequal(tmp_type, str_type)) {
            h5tools_str_append(buffer, "H5T_C_S1;\n");
            goto done;
        }

        /* If not equal to C variable-length string, check Fortran type. */
        H5Tclose(str_type);
        str_type = H5Tcopy(H5T_FORTRAN_S1);
        H5Tset_cset(str_type, cset);
        H5Tset_size(str_type, size);
        H5Tset_strpad(str_type, str_pad);

        /* Are the two types equal? */
        if (H5Tequal(tmp_type, str_type)) {
            h5tools_str_append(buffer, "H5T_FORTRAN_S1;\n");
            goto done;
        }

        /* Change the endianness and see if they're equal. */
        order = H5Tget_order(tmp_type);
        if (order == H5T_ORDER_LE)
            H5Tset_order(str_type, H5T_ORDER_LE);
        else if (order == H5T_ORDER_BE)
            H5Tset_order(str_type, H5T_ORDER_BE);

        if (H5Tequal(tmp_type, str_type)) {
            h5tools_str_append(buffer, "H5T_FORTRAN_S1;\n");
            goto done;
        }

        /* Type doesn't match any of above. */
        h5tools_str_append(buffer, "unknown_one_character_type;\n ");

        done: H5Tclose(str_type);
        H5Tclose(tmp_type);

        ctx->indent_level--;
        h5tools_str_append(buffer, "%s",
                h5tools_dump_header_format->strblockend);
        break;

    case H5T_BITFIELD:
        if (H5Tequal(type, H5T_STD_B8BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_B8BE");
        }
        else if (H5Tequal(type, H5T_STD_B8LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_B8LE");
        }
        else if (H5Tequal(type, H5T_STD_B16BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_B16BE");
        }
        else if (H5Tequal(type, H5T_STD_B16LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_B16LE");
        }
        else if (H5Tequal(type, H5T_STD_B32BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_B32BE");
        }
        else if (H5Tequal(type, H5T_STD_B32LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_B32LE");
        }
        else if (H5Tequal(type, H5T_STD_B64BE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_B64BE");
        }
        else if (H5Tequal(type, H5T_STD_B64LE) == TRUE) {
            h5tools_str_append(buffer, "H5T_STD_B64LE");
        }
        else {
            h5tools_str_append(buffer, "undefined bitfield");
        }
        break;

    case H5T_OPAQUE:
        h5tools_str_append(buffer, "\n");
        h5tools_str_append(buffer, "H5T_OPAQUE;\n");
        h5tools_str_append(buffer, "OPAQUE_TAG \"%s\";\n", H5Tget_tag(type));
        break;

    case H5T_COMPOUND:
        nmembers = H5Tget_nmembers(type);
        h5tools_str_append(buffer, "H5T_COMPOUND %s\n",
                h5tools_dump_header_format->structblockbegin);

        for (i = 0; i < nmembers; i++) {
            mname = H5Tget_member_name(type, i);
            mtype = H5Tget_member_type(type, i);

            if (H5Tget_class(mtype) == H5T_COMPOUND)
                ctx->indent_level++;

            h5tools_print_datatype(buffer, info, ctx, mtype);

            if (H5Tget_class(mtype) == H5T_COMPOUND)
                ctx->indent_level--;

            h5tools_str_append(buffer, " \"%s\";\n", mname);
            free(mname);
        }

        h5tools_str_append(buffer, "%s",
                h5tools_dump_header_format->structblockend);
        break;

    case H5T_REFERENCE:
        h5tools_str_append(buffer, "H5T_REFERENCE");
        break;

    case H5T_ENUM:
        h5tools_str_append(buffer, "H5T_ENUM %s\n",
                h5tools_dump_header_format->enumblockbegin);
        ctx->indent_level++;
        super = H5Tget_super(type);
        h5tools_print_datatype(buffer, info, ctx, super);
        h5tools_str_append(buffer, ";\n");
        h5tools_print_enum(buffer, info, ctx, type);
        ctx->indent_level--;
        h5tools_str_append(buffer, "%s",
                h5tools_dump_header_format->enumblockend);
        break;

    case H5T_VLEN:
        h5tools_str_append(buffer, "H5T_VLEN %s ",
                h5tools_dump_header_format->vlenblockbegin);
        super = H5Tget_super(type);
        h5tools_print_datatype(buffer, info, ctx, super);
        H5Tclose(super);

        /* Print closing */
        h5tools_str_append(buffer, "%s",
                h5tools_dump_header_format->vlenblockend);
        break;

    case H5T_ARRAY:
        /* Get array base type */
        super = H5Tget_super(type);

        /* Print lead-in */
        h5tools_str_append(buffer, "H5T_ARRAY { ");

        /* Get array information */
        ndims = H5Tget_array_ndims(type);
        H5Tget_array_dims2(type, dims);

        /* Print array dimensions */
        for (i = 0; i < ndims; i++)
            h5tools_str_append(buffer, "[%d]", (int) dims[i]);

        h5tools_str_append(buffer, " ");

        /* Print base type */
        h5tools_print_datatype(buffer, info, ctx, super);

        /* Close array base type */
        H5Tclose(super);

        /* Print closing */
        h5tools_str_append(buffer, " }");

        break;

    default:
        h5tools_str_append(buffer, "unknown datatype");
        break;
    }
}

/*-------------------------------------------------------------------------
 * Function:    print_enum
 *
 * Purpose:     prints the enum data
 *
 * Return:      void
 *
 * Programmer:  Patrick Lu
 *
 * Modifications:
 *
 * NOTE: this function was taken from h5ls. should be moved into the toolslib
 *
 *-----------------------------------------------------------------------*/
void h5tools_print_enum(h5tools_str_t *buffer/*in,out*/,
        const h5tool_format_t *info, h5tools_context_t *ctx/*in,out*/,
        hid_t type) {
    char **name = NULL; /*member names                   */
    unsigned char *value = NULL; /*value array                    */
    unsigned char *copy = NULL; /*a pointer to value array       */
    unsigned nmembs; /*number of members              */
    int nchars; /*number of output characters    */
    hid_t super; /*enum base integer type         */
    hid_t native = -1; /*native integer datatype        */
    size_t dst_size; /*destination value type size    */
    unsigned i;

    nmembs = H5Tget_nmembers(type);
    assert(nmembs > 0);
    super = H5Tget_super(type);

    /*
     * Determine what datatype to use for the native values.  To simplify
     * things we entertain three possibilities:
     *  1. long long -- the largest native signed integer
     *    2. unsigned long long -- the largest native unsigned integer
     *    3. raw format
     */
    if (H5Tget_size(type) <= sizeof(long long)) {
        dst_size = sizeof(long long);

        if (H5T_SGN_NONE == H5Tget_sign(type)) {
            native = H5T_NATIVE_ULLONG;
        }
        else {
            native = H5T_NATIVE_LLONG;
        }
    }
    else {
        dst_size = H5Tget_size(type);
    }

    /* Get the names and raw values of all members */
    name = calloc(nmembs, sizeof(char *));
    value = calloc(nmembs, MAX(H5Tget_size(type), dst_size));

    for (i = 0; i < nmembs; i++) {
        name[i] = H5Tget_member_name(type, i);
        H5Tget_member_value(type, i, value + i * H5Tget_size(type));
    }

    /* Convert values to native datatype */
    if (native > 0)
        H5Tconvert(super, native, nmembs, value, NULL, H5P_DEFAULT);

    /*
     * Sort members by increasing value
     *    ***not implemented yet***
     */

    /* Print members */
    for (i = 0; i < nmembs; i++) {
        h5tools_str_append(buffer, "\"%s\"", name[i]);
        nchars = strlen(name[i]);
        h5tools_str_append(buffer, "%*s   ", MAX(0, 16 - nchars), "");

        if (native < 0) {
            size_t j;

            h5tools_str_append(buffer, "0x");

            for (j = 0; j < dst_size; j++)
                h5tools_str_append(buffer, "%02x", value[i * dst_size + j]);
        }
        else if (H5T_SGN_NONE == H5Tget_sign(native)) {
            /*On SGI Altix(cobalt), wrong values were printed out with "value+i*dst_size"
             *strangely, unless use another pointer "copy".*/
            copy = value + i * dst_size;
            h5tools_str_append(buffer, "%" H5_PRINTF_LL_WIDTH "u",
                    *((unsigned long long *) ((void *) copy)));
        }
        else {
            /*On SGI Altix(cobalt), wrong values were printed out with "value+i*dst_size"
             *strangely, unless use another pointer "copy".*/
            copy = value + i * dst_size;
            h5tools_str_append(buffer, "%" H5_PRINTF_LL_WIDTH "d",
                    *((long long *) ((void *) copy)));
        }

        h5tools_str_append(buffer, ";\n");
    }

    /* Release resources */
    for (i = 0; i < nmembs; i++)
        free(name[i]);

    free(name);
    free(value);
    H5Tclose(super);

    if (0 == nmembs)
        h5tools_str_append(buffer, "\n<empty>");
}

/*-------------------------------------------------------------------------
 * Function:    dump_datatype
 *
 * Purpose:     Dump the datatype. Datatype can be HDF5 predefined
 *              atomic datatype or committed/transient datatype.
 *
 * Return:      void
 *
 * Programmer:  Ruey-Hsia Li
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void h5tools_dump_datatype(FILE *stream, const h5tool_format_t *info,
        h5tools_context_t *ctx/*in,out*/, unsigned flags, hid_t type) {
    size_t ncols = 80; /*available output width */
    h5tools_str_t buffer; /*string into which to render */
    int multiline; /*datum was multiline  */
    hsize_t curr_pos; /* total data element position   */
    hsize_t elmt_counter = 0;/*counts the # elements printed.*/

    /* setup */
    HDmemset(&buffer, 0, sizeof(h5tools_str_t));

    if (info->line_ncols > 0)
        ncols = info->line_ncols;

    /* pass to the prefix in h5tools_simple_prefix the total position
     * instead of the current stripmine position i; this is necessary
     * to print the array indices
     */
    curr_pos = ctx->sm_pos;

    h5tools_simple_prefix(stream, info, ctx, curr_pos, 0);
    /* Render the element */
    h5tools_str_reset(&buffer);

    ctx->indent_level++;
    h5tools_str_append(&buffer, "%s %s ",
            h5tools_dump_header_format->datatypebegin,
            h5tools_dump_header_format->datatypeblockbegin);

    h5tools_print_datatype(&buffer, info, ctx, type);

    if (HDstrlen(h5tools_dump_header_format->datatypeblockend)) {
        h5tools_str_append(&buffer, "%s",
                h5tools_dump_header_format->datatypeblockend);
        if (HDstrlen(h5tools_dump_header_format->datatypeend))
            h5tools_str_append(&buffer, " ");
    }
    if (HDstrlen(h5tools_dump_header_format->datatypeend))
        h5tools_str_append(&buffer, "%s",
                h5tools_dump_header_format->datatypeend);
    h5tools_str_append(&buffer, "\n");

    curr_pos = h5tools_render_element(stream, info, ctx, &buffer, curr_pos,
            flags, ncols, elmt_counter, 0);

    ctx->need_prefix = TRUE;
    ctx->indent_level--;
}

/*-------------------------------------------------------------------------
 * Function:    init_acc_pos
 *
 * Purpose:     initialize accumulator and matrix position
 *
 * Return:      void
 *
 * Programmer:  pvn
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void init_acc_pos(h5tools_context_t *ctx, hsize_t *dims) {
    int i;

    assert(ctx->ndims);

    ctx->acc[ctx->ndims - 1] = 1;
    for (i = (ctx->ndims - 2); i >= 0; i--) {
        ctx->acc[i] = ctx->acc[i + 1] * dims[i + 1];
    }
    for (i = 0; i < ctx->ndims; i++)
        ctx->pos[i] = 0;
}

/*-------------------------------------------------------------------------
 * Function: do_bin_output
 *
 * Purpose: Dump memory buffer to a binary file stream
 *
 * Return: Success:    SUCCEED
 *         Failure:    FAIL
 *
 * Programmer: Pedro Vicente Nunes
 *             Friday, June 2, 2006
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static
int do_bin_output(FILE *stream, hsize_t nelmts, hid_t tid, void *_mem) {
    unsigned char *mem = (unsigned char*) _mem;
    size_t size; /* datum size */
    hsize_t i; /* element counter  */

    size = H5Tget_size(tid);

    for (i = 0; i < nelmts; i++) {
        if (render_bin_output(stream, tid, mem + i * size) < 0) {
            printf("\nError in writing binary stream\n");
            return FAIL;
        }
    }

    return SUCCEED;
}

/*-------------------------------------------------------------------------
 * Function: render_bin_output
 *
 * Purpose: Write one element of memory buffer to a binary file stream
 *
 * Return: Success:    SUCCEED
 *         Failure:    FAIL
 *
 * Programmer: Pedro Vicente Nunes
 *             Friday, June 2, 2006
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static
int render_bin_output(FILE *stream, hid_t tid, void *_mem) {
    unsigned char *mem = (unsigned char*) _mem;
    size_t size; /* datum size */
    float tempfloat;
    double tempdouble;
    unsigned long long tempullong;
    long long templlong;
    unsigned long tempulong;
    long templong;
    unsigned int tempuint;
    int tempint;
    unsigned short tempushort;
    short tempshort;
    unsigned char tempuchar;
    char tempschar;
#if H5_SIZEOF_LONG_DOUBLE !=0
    long double templdouble;
#endif
#ifdef DEBUG_H5DUMP_BIN
    static char fmt_llong[8], fmt_ullong[8];
    if (!fmt_llong[0]) {
        sprintf(fmt_llong, "%%%sd", H5_PRINTF_LL_WIDTH);
        sprintf(fmt_ullong, "%%%su", H5_PRINTF_LL_WIDTH);
    }
#endif

    size = H5Tget_size(tid);

    if (H5Tequal(tid, H5T_NATIVE_FLOAT)) {
        memcpy(&tempfloat, mem, sizeof(float));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%g ", tempfloat);
#else
        if (1 != fwrite(&tempfloat, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_DOUBLE)) {
        memcpy(&tempdouble, mem, sizeof(double));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%g ", tempdouble);
#else
        if (1 != fwrite(&tempdouble, size, 1, stream))
            return FAIL;
#endif
    }
#if H5_SIZEOF_LONG_DOUBLE !=0
    else if (H5Tequal(tid, H5T_NATIVE_LDOUBLE)) {
        memcpy(&templdouble, mem, sizeof(long double));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%Lf ", templdouble);
#else
        if (1 != fwrite(&templdouble, size, 1, stream))
            return FAIL;
#endif
    }
#endif
    else if (H5T_STRING == H5Tget_class(tid)) {
        unsigned int i;
        H5T_str_t pad;
        char *s;

        pad = H5Tget_strpad(tid);

        if (H5Tis_variable_str(tid)) {
            s = *(char**) mem;
            if (s != NULL)
                size = HDstrlen(s);
        }
        else {
            s = (char *) mem;
            size = H5Tget_size(tid);
        }
        for (i = 0; i < size && (s[i] || pad != H5T_STR_NULLTERM); i++) {
            memcpy(&tempuchar, &s[i], sizeof(unsigned char));
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, "%d", tempuchar);
#else
            if (1 != fwrite(&tempuchar, size, 1, stream))
                return FAIL;
#endif
        } /* i */
    }
    else if (H5Tequal(tid, H5T_NATIVE_INT)) {
        memcpy(&tempint, mem, sizeof(int));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%d ", tempint);
#else
        if (1 != fwrite(&tempint, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_UINT)) {
        memcpy(&tempuint, mem, sizeof(unsigned int));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%u ", tempuint);
#else
        if (1 != fwrite(&tempuint, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_SCHAR)) {
        memcpy(&tempschar, mem, sizeof(char));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%d ", tempschar);
#else
        if (1 != fwrite(&tempschar, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_UCHAR)) {
        memcpy(&tempuchar, mem, sizeof(unsigned char));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%u ", tempuchar);
#else
        if (1 != fwrite(&tempuchar, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_SHORT)) {
        memcpy(&tempshort, mem, sizeof(short));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%d ", tempshort);
#else
        if (1 != fwrite(&tempshort, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_USHORT)) {
        memcpy(&tempushort, mem, sizeof(unsigned short));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%u ", tempushort);
#else
        if (1 != fwrite(&tempushort, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_LONG)) {
        memcpy(&templong, mem, sizeof(long));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%ld ", templong);
#else
        if (1 != fwrite(&templong, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_ULONG)) {
        memcpy(&tempulong, mem, sizeof(unsigned long));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, "%lu ", tempulong);
#else
        if (1 != fwrite(&tempulong, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_LLONG)) {
        memcpy(&templlong, mem, sizeof(long long));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, fmt_llong, templlong);
#else
        if (1 != fwrite(&templlong, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_ULLONG)) {
        memcpy(&tempullong, mem, sizeof(unsigned long long));
#ifdef DEBUG_H5DUMP_BIN
        fprintf(stream, fmt_ullong, tempullong);
#else
        if (1 != fwrite(&tempullong, size, 1, stream))
            return FAIL;
#endif
    }
    else if (H5Tequal(tid, H5T_NATIVE_HSSIZE)) {
        if (sizeof(hssize_t) == sizeof(int)) {
            memcpy(&tempint, mem, sizeof(int));
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, "%d ", tempint);
#else
            if (1 != fwrite(&tempint, size, 1, stream))
                return FAIL;
#endif
        }
        else if (sizeof(hssize_t) == sizeof(long)) {
            memcpy(&templong, mem, sizeof(long));
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, "%ld ", templong);
#else
            if (1 != fwrite(&templong, size, 1, stream))
                return FAIL;
#endif
        }
        else {
            memcpy(&templlong, mem, sizeof(long long));
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, fmt_llong, templlong);
#else
            if (1 != fwrite(&templlong, size, 1, stream))
                return FAIL;
#endif
        }
    }
    else if (H5Tequal(tid, H5T_NATIVE_HSIZE)) {
        if (sizeof(hsize_t) == sizeof(int)) {
            memcpy(&tempuint, mem, sizeof(unsigned int));
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, "%u ", tempuint);
#else
            if (1 != fwrite(&tempuint, size, 1, stream))
                return FAIL;
#endif
        }
        else if (sizeof(hsize_t) == sizeof(long)) {
            memcpy(&tempulong, mem, sizeof(unsigned long));
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, "%lu ", tempulong);
#else
            if (1 != fwrite(&tempulong, size, 1, stream))
                return FAIL;
#endif
        }
        else {
            memcpy(&tempullong, mem, sizeof(unsigned long long));
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, fmt_ullong, tempullong);
#else
            if (1 != fwrite(&tempullong, size, 1, stream))
                return FAIL;
#endif
        }
    }
    else if (H5Tget_class(tid) == H5T_COMPOUND) {
        unsigned j;
        hid_t memb;
        unsigned nmembs;
        size_t offset;

        nmembs = H5Tget_nmembers(tid);

        for (j = 0; j < nmembs; j++) {
            offset = H5Tget_member_offset(tid, j);
            memb = H5Tget_member_type(tid, j);

            if (render_bin_output(stream, memb, mem + offset) < 0)
                return FAIL;

            H5Tclose(memb);
        }
    }
    else if (H5Tget_class(tid) == H5T_ENUM) {
        unsigned int i;
        if (1 == size) {
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, "0x%02x", mem[0]);
#else
            if (1 != fwrite(&mem[0], size, 1, stream))
                return FAIL;
#endif
        }
        else {
            for (i = 0; i < size; i++) {
#ifdef DEBUG_H5DUMP_BIN
                fprintf(stream, "%s%02x", i?":":"", mem[i]);
#else
                if (1 != fwrite(&mem[i], sizeof(char), 1, stream))
                    return FAIL;
#endif
            } /*i*/
        }/*else 1 */
    }
    else if (H5Tget_class(tid) == H5T_ARRAY) {
        int k, ndims;
        hsize_t i, dims[H5S_MAX_RANK], temp_nelmts, nelmts;
        hid_t memb;

        /* get the array's base datatype for each element */
        memb = H5Tget_super(tid);
        size = H5Tget_size(memb);
        ndims = H5Tget_array_ndims(tid);
        H5Tget_array_dims2(tid, dims);
        assert(ndims >= 1 && ndims <= H5S_MAX_RANK);

        /* calculate the number of array elements */
        for (k = 0, nelmts = 1; k < ndims; k++) {
            temp_nelmts = nelmts;
            temp_nelmts *= dims[k];
            nelmts = (size_t) temp_nelmts;
        }

        /* dump the array element */
        for (i = 0; i < nelmts; i++) {
            if (render_bin_output(stream, memb, mem + i * size) < 0)
                return FAIL;
        }

        H5Tclose(memb);
    }
    else if (H5Tget_class(tid) == H5T_VLEN) {
        unsigned int i;
        hsize_t nelmts;
        hid_t memb;

        /* get the VL sequences's base datatype for each element */
        memb = H5Tget_super(tid);
        size = H5Tget_size(memb);

        /* Get the number of sequence elements */
        nelmts = ((hvl_t *) mem)->len;

        for (i = 0; i < nelmts; i++) {
            /* dump the array element */
            if (render_bin_output(stream, memb, ((char *) (((hvl_t *) mem)->p))
                    + i * size) < 0)
                return FAIL;
        }
        H5Tclose(memb);
    }
    else {
        size_t i;
        if (1 == size) {
#ifdef DEBUG_H5DUMP_BIN
            fprintf(stream, "0x%02x", mem[0]);
#else
            if (1 != fwrite(&mem[0], size, 1, stream))
                return FAIL;
#endif
        }
        else {
            for (i = 0; i < size; i++) {
#ifdef DEBUG_H5DUMP_BIN
                fprintf(stream, "%s%02x", i?":":"", mem[i]);
#else
                if (1 != fwrite(&mem[i], sizeof(char), 1, stream))
                    return FAIL;
#endif
            } /*i*/
        }/*else 1 */
    }

    return SUCCEED;
}

/*-------------------------------------------------------------------------
 * Function:    h5tools_is_zero
 *
 * Purpose: Determines if memory is initialized to all zero bytes.
 *
 * Return:  TRUE if all bytes are zero; FALSE otherwise
 *
 * Programmer:  Robb Matzke
 *              Monday, June  7, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hbool_t h5tools_is_zero(const void *_mem, size_t size) {
    const unsigned char *mem = (const unsigned char *) _mem;

    while (size-- > 0)
        if (mem[size])
            return FALSE;

    return TRUE;
}


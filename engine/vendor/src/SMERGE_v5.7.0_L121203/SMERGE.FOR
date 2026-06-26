c----------------------------------------------------------------------
c --- SMERGE -- Surface Meteorological Preprocessor
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0    Level: 121203              MAIN
c
c --- Written by:  E. Insley, J. Scire
c 
c     Copyright (c) 2014 by Exponent, Inc.
c
c --- Reads 'N' CD144, SAMSON, HUSWO, ISHWO, TD3505, TD9956, or GENERIC
c --- surface observation files containing data for one station each and  
C --- reformats the data into 1 file containing 'N' stations sorted by 
C --- time.  Formatted files may also be combined with an already existing
c --- SMERGE (SURF.DAT) file.  The output file may be formatted or
c --- unformatted.  If the output SURF.DAT file is unformatted, it may
c --- be either in a packed (compressed) or unpacked form.
c --- Optionally it can also output a VSRN.DAT file of present weather
c --- codes and visibilty for subsequent use in CALPOST for visibility
c --- calculations using Method 7.
c
c **********************************************************************
c --- Exponent, Inc. Updates:
c **********************************************************************
c --- V5.661, Level 110225 to V5.7.0, Level: 121203
c     - Revise processing of ISH data quality flags that were integer
c       (0-9) but now have character entries (0-9 or A-Z).  At this
c       time the character codes indicate that the corresponding data
c       values may be used.  The code update screens for known values
c       and STOPS if new characters are encountered.  Also, revise ISH
c       interpretation of quality flag = '9' to indicate that the flag
c       may be missing, not the data.
c       This is clarified by the data format document dated Nov. 7, 2012
c       which removed the '9=missing' assignment that had conflicted
c       with the '9: Passed gross limits check if element is present'
c       assignment.  Note that some additional-codes data types still
c       list a quality flag entry of 9 that is 'missing', and that this
c       is also interpreted as 'missing qc flag'.
c       Modified:  READISH, ISHQC
c     - Increase list-file version string output to a12
c       Modified:  READCF
c     - Missing ISH temperatures are 999.9, so reset check from 9999.8
c       to 999.8
c       Modified:  READISH
c     - Recognize and skip additional codes in ISHWO file:
c       AU, AX
c       Precip section
c       AB, AD, AE, AH, AI, AK, AM, AN, AO, AP
c       Climate section
c       CB, CF, CG, CH, CI, CN, CO, CR, CT, CU, CV, CW, CX
c       Cloud & Solar section
c       GH, GK, GL, GM, GN, GO, GP, GQ, GR
c       Ground section
c       IB, IC
c       Temperature section
c       KB, KC, KD, KE, KF
c       Pressure section
c       MG, MH, MK
c       Wind section
c       OB, OE
c       Relative Humidity section
c       RHX
c       Soil Temperature section
c       ST
c       Modified:  READISH
c **********************************************************************
c
c --- Updated V5.661 (110225) from V5.66 (100713) (D. Strimaitis)
c     2) Updated CALUTILS.FOR to version v2.58 (110225)
c        - Add control file variable type 5 (character array) and
c          retain commas in string returned so that array of
c          variable values can be parsed
c        Modified: READIN, ALTONU, SETVAR
c
c --- Updated V5.66 (100713) from v5.652 (090511) (J. Scire)
c     - Include table of valid data by variable and station with
c       hours of valid data and percentage.  Also include hours
c       and percentage of calm winds at each station.
c       Modified:  RDWRITS, BLOCK DATA
c                  PARAMS.SMG
c       New Common block:  /VALID/
c
c --- Updated V5.652(090511) from V5.651(080407)    (D. Strimaitis)
c     - CALUTILS from v2.56 Level 080407 to v2.571 Level 090511
c       Increase control file line length to 200 characters
c       Activate CPU clock using F95 system routine
c       Add routine to reformat a date string
c       New     : FMT_DATE
c       Modified: PARAMS.CAL, READIN, DATETM
c     - Reformat date reported to list file
c       Modified: FIN
c
c --- Updated V5.651(080407) from V5.65(080229)    (D. Strimaitis)
c     - Updated CALUTILS from v2.55 Level 070327 to v2.56 Level 080407
c       Control file entries in exponential notation were not correct
c       if decimal point was missing (2e-02 was read as 0.2e-02).
c       Modified: ALTONU
c
c --- Updated V5.65(080229) from V5.64(070912)    (D. Strimaitis)
c     - Revise time-window in READISH so that only reported times that
c       fall within the hour (HH-1)01 to HH00 are assigned to hour HH.
c       For example, observations at 1701, 1725, 1749, and 1800 are all
c       assigned to "hour 18".
c     - Fix logic in READISH that overstates the number of missing
c       hours.
c       Modified:  READISH
c     - Add a check on units provided in header for generic format
c     - The format of the GENERIC format was updated to a version 2.0
c       A line was added in the header.
c       Modified: COMP  (C. Escoffier-Czaja)
c
c --- Updated V5.64(070912) from V5.63(070327)    (C. Escoffier-Czaja)
c     - Add an option to read generic format surface station.
c       a sample format is provided (sample_generic_v2.csv) - missing are
c       given as 9999.
c       Modified: COMP, RDWRITS, READCF
c
c --- Updated V5.63(070327) from V5.62(060519)    (D. Strimaitis)
c     - Fix list-file time zone identification (all zones reported as
c       zero)
c       Modified:  READCF
c     - CALUTILS from v2.52 Level 060519 to v2.55 Level 070327
c       Move GLOBE1 to COORDLIB
c       Allow negative increments in INCRS
c       Fixed format bug in subroutine BASRUTC for the case of time
c       zone zero (output string was 'UTC+0  0' instead of 'UTC+0000'
c       Modified:  INCRS, UTCBASR, BASRUTC
c       Removed:   GLOBE1
c
c --- Updated V5.62(060519) from V5.61(060309)    (D. Strimaitis)
c     - CALUTILS from v2.51 Level 051019 to v2.52 Level 060519
c       Variable names in control file are not processed correctly
c       if there are too many characters (including blanks) to the
c       left of the "=" sign (run stops in setup phase).
c       Modified:  READIN
c
c --- Updated V5.61(060309) from V5.601(051005)    (D. Strimaitis)
c     - Updated to CALUTILS V2.51 (051019) from V2.5 (041123)
c     - Filnames changed from c*70 to c*132 (for CALUTILS V2.3 and later)
c       Modified:  FILNAM.SMG
c                  READCF
c
c --- Updated V5.60 (050921) to V5.601(051005)(F.Robe)
c     - Fixed typo in RDSN which induced (Ending Day n Hour24) to be  
c       interpreted as (day n, hour 23) instead of (day n+1, hour 0) 
c
c --- Updated V5.6 (041123) to V5.60(050921) (F.Robe)
c     - bug fix in READCF  so that for beg/ending hour of 24
c       the beg/ending day is incremented when the hour is reset to 0
c
c --- Updated V5.6 (041123) from V5.54(041102) (F.Robe)
c     - Introduce new time format with beginning/ending times for
c       each records, including seconds , and UTC Time zone 
c       (character string) (SURF.DAT version 2.1)
c     - Changes only to the input/output subroutines, not to the main
c       computational subroutines. Hourly records only at this point
c     - Changes to SMERGE.INP: explicit beg/ending times with seconds
c       and explicit time zone relatived to UTC. NEw input parameters
c       IBSEC,IESEC . ASTZ and ABTZ (char. strings with UTC time zone)
c       replace XSTZ and XBTZ respectively   .
c       However SMERGE can still read old SMERGE.INP files (with
c       hour-ending beg/ending times (no seconds) and ASTZ, XBTZ time zone
c     - Character time zone stored in CONTROL.SMG
c       CAll to new CALUTILS subroutines (BASRUTC,UTCBASR) to flip between
c       XBTZ (real base time zone) and AXTZ (character UTC time zone)
c     - Changes to READCF,RDHD,WRHD,WRHDP,WRS,WRP
c     - New subroutine RDSN to read SURF.DAT version 2.1 (explicit time, UTC)
c     - Replace array assignments of IPWOUT/IPWOUT1/ISAMPW by explicit  
c       component assignments to allow Fortran 77 compilation 

c
c --- Updated V5.54(041102) from V5.53(041029)
c     - Fixed bug in RDWRITS that assigned the wrong station
c       elevation array element when adding stations to an
c       existing SURF.DAT file.                    (D. Strimaitis)
c
c --- Updated V5.53(041029) from V5.52(041026)
c     - Modified End-of-File response after call to GETISH to continue
c       processing valid data on the last record of a file.  Missing
c       station pressure on the last record was not calculated in 
c       previous version (JDAT=5,6,7).             (D. Strimaitis)
c
c --- Updated V5.52(041026) from V5.51(040922)
c     - Added station ID check (the ID provided in the control file for
c       each station file processed must match the ID found in the
c       corresponding file.                         (D. Strimaitis)
c     - Disable the VSRN output option pending revisions
c                                                   (D. Strimaitis)
c     - Fixed bug in the format for reading TD3505 and TD9956 data
c       (types 6 & 7) in READISH                    (K. Morrison)
c     - Fixed bug in call to GETISH in RDWRITS that used the wrong
c       station time zone array element when adding stations to an
c       existing SURF.DAT file.                     (K. Morrison)
c     - Require time zone XSTZ entered in control file to be 0 for
c       ISHWO, TD3505, and TD9956                   (K. Morrison)
c
c --- Updated V5.51(040922) from V5.5_draft(040709) (K. Morrison)
c     - Modifications in RDWRITS
c       - Correction to empirical calculations for pressure
c       - Remove substitution of total cloud cover for missing opaque
c         for CD144
c       - Correction to station identification for SAMSON and HUSWO
c       - Correction to precip codes for SAMSON
c       - Identify HUSWO as English or metric units based on pressure
c         value, and adjust unit conversions in consequence
c     - Change dimension of ncbdf from 8 to 7 in WRS
c     - Add control file variable IHUSWO to designate either English or
c       metric data units in HUSWO file (default=English).  Keep logic
c       to determine likely units from data ranges, but stop if it
c       appears that units are wrong.                            (DGS)
c
c --- Updated V5.5_draft(040709) from V5.44(040621) (K. Morrison)
c     - Add TD3505 and TD9956 data types (types 6 & 7)
c     - Get present weather codes, and standardize to general manual
c       DATSAV station codes
c     - Read visibility for all data types
c     - Add option to output a VSRN.DAT file
c
c --- Updated V5.44(040621) from V5.43(040322) (K. Morrison)
c     - Utility programs added in separate file (PRESUTIL.FOR V:1.00)
c       for conversions among pressure variables (station, altimeter, 
c       sea-level), called from RDWRITS
c     - Added reading of dew point for all data types and sea-level
c       pressure for CD-144 in RDWRITS
c     - Added ability to calculate pressure from sea-level pressure for
c       CD-144 data in subroutine RDWRITS
c     - Added ability to calculate humidity from dry-bulb and dew point
c       for data types other than ISHWO in RDWRITS (already done for this 
c       type in READISH)
c     - Moved pressure calculation from READISH to RDWRITS for ISHWO data,
c       and added passing of altimeter and sea-level back to RDWRITS
c     - Added new logical to CONTROL common and Input Group 1 (LPCALC) to 
c       choose replacement of missing pressure values with calculated 
c       values based on sea-level pressure - this variable automatically 
c       enabled for ISHWO files
c
c --- Updated V5.43(040322) from V5.42(040318) (K. Morrison)
c     - Correction to pressure estimation for ISHWO data
c
c --- Updated V5.42(040318) from V5.41(040210) (K. Morrison)
c     - Correction to CD-144 treatment of "0" wind direction
c     - Correction to ISHWO variable wind direction
c
c --- Updated V5.41(040210) from V5.4(040205) (K. Morrison)
c     - Corrections and enhancements to processing of ISHWO data
c
c --- Updated V5.4(040205) from V5.31(030528) (K. Morrison)
c     - Added processing of ISHWO data
c
c --- Updated V5.31(030528) from V5.3(030402) (D. Strimaitis)
c     - Updated CALUTILS (Version 2.2, Level 030528)
c
c --- Updated V5.3(030402) from V5.2(020828) (D. Strimaitis)
c     - Updated CALUTILS (Version 2.1, Level 030402)
c     - New header for output data file (SURF.DAT)
c             ENVPOP=0  Revised SURF.DAT header
c             ENVPOP=1  Revised SURF.DAT header with station locations
c
c --- Updated V5.2(020828) from V5.1(020809) (D. Strimaitis)
c     - Updated CALUTILS (Version 1.1, Level 020828)
c
c --- Updated V5.1(020809) from V5.0(020308) (D. Strimaitis)
c     - Add parameter ENVPOP to params.smg and use this to toggle
c       environment configurations (these will evolve in time):
c             ENVPOP=0  SURF.DAT header as in 1/2000 Users Guide
c             ENVPOP=1  SURF.DAT header with draft station locations
c                       introduced in V5.0(020308)
c
c --- Updated V5.0(020308) from V5.0(010630) (D. Strimaitis)
c     - Allow station ID to exceed 5 digits (use 10 for now in format
c       statements)
c     - Initialize data to missing at start of each hour
c     - New header format for SURF.DAT:  include SMERGE version, level,
c       station name, ID, anemometer height, and LAT/LON
c     - Read station information from text file
c     - Modified CD144 Format (JDAT=4): look for extended CD144 record
c       to obtain precip rate, and create a PRECIP.DAT output file
c
c --- Updated V5.0(010630) from V4.0(010315) (D. Strimaitis)
c     - Restructure inputs for CALPUFF system control file
c     - Restructure main program as subroutine COMP
c     - Place parameters and commons into include files
c     - Place system-wide utilities into an include module (calutil.for)
c
c --- Updated 010315 from 991223 (D. Strimaitis)
c     - Y2K treatment of year enforced
c
c --- Updated 991223 from 970718 (Benjamin de Foy)
c     - JDAT = 3: HUSWO format
c     - Count & print no. of hours with data missing for all stations
c     - Some restructuring in subroutine rdwrits
c     - Add 'backspace' command to deal with missing hours in input
c       files for all three formats
c     - Subroutine Clouds replaced - simpler + more robust
c     - Only col 25 of CD144 was used for pcodes, now uses cols 25-29
c     - Changed surf.dat formats statement - fewer digits precision
c
c --- Updated 7/18/97 from V4.0, L961113 to V4.0, L970718 (E. Insley)
c     - Fixed hour handling for CD-ROM data in SUBR. RDWRITS.  It had
c       subtracted one from the hour for all hours which incorrectly
c       shifted the data.  It now only changes hr 24 to hr 0 of the
c       next day without shifting the data.
c
c --- Updated 11/13/96 from V3.1, L961031 to V3.1, L961113 (E. Insley)
c     - Modified input structure to allow the user to name the SURF.DAT
c       output file and the previous SMERGE data file used as input
c     - Changed file names from Char*10 to Char*12
c
c --- Updated 10/31/96 from V3.1, L961014 to V3.1, L961031 (J. Scire)
c     - Add skip of two header records when using SAMSON format
c
c --- Updated 10/14/96 from V3.0, L941215 to V3.1, L961014 (J. Scire)
c     - Added QA checks to input variables,
c     - Added warning message to screen whenever a fatal,
c       program-generated error occurs
c     - Added addition description of inputs,
c     - Changed SURF.DAT status from "NEW" to "UNKNOWN"
c     - Changed SMERGE.LST status from unspecified to "UNKNOWN"
c     - Fixed error in DO 85 loop - checks for blank station ID field
c
c --- Updated by:  E. Insley, SRC  11/18/94
c   - Added option to read an existing 'SURF.DAT' file as either
c   - formatted or unformatted (previously it was only unformatted).
c   - Modified the MAIN program and Subroutines OPENS, RDWRITS and RDS.
c
c --- Updated by:  E. Insley, SRC  3/30/94
c   - Added option to create a formatted 'SURF.DAT' file.
c   - Modified the MAIN program and Subroutines OPENS, RDWRITS and WRHD.
c
c --- Updated by:  R. Mentzer, SRC  9/1/92
c   - Allow SMERGE to process both CD144 and SAMSON files.
c   - Allow missing data to pass through with a missing value indicator.
c   - Modified subroutines RDWRITS and CLOUDS.
c----------------------------------------------------------------------
      Program SMERGE
c
c --- Include parameters
      include 'params.smg'
c --- Include common blocks
      include 'qa.smg'
c
c --- Set version and level number of program
      ver='5.7.0'
      level='121203'
c
c --- SETUP PHASE -- read control file information
      call SETUP
c
c --- COMPUTATIONAL PHASE -- loop over processing periods
      call COMP
c
c --- TERMINATION PHASE -- program termination functions
      call FIN(2)
c
      stop
      end
c----------------------------------------------------------------------
      BLOCK DATA
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 100713       BLOCK DATA
c                D. Strimaitis, Earth Tech, Inc.
c
c --- Include parameter statements
      include 'params.smg'
c
c --- Include common blocks
      include 'filnam.smg'
      include 'control.smg'
      include 'qa.smg'
      include 'valid.smg'

c --- FILNAM common block
      data runinp/'smerge.inp'/,runlst/'smerge.lst'/,
     1 prevdat/'prev.dat'/,surfdat/'surf.dat'/,
     2 sfcmet/'none.dat'/,sstatxt/'surf.txt'/,vsrndat/'vsrn.dat'/
c --- FILLOG common block
      data lcfiles/.true./

c --- CONTROL common block
      data nbstn/0/, inform/2/, ioform/2/, iopack/0/, jtxt/1/,
     1 lpcalc/.false./, lvsrnout/.false./
      data ihuswo/1/

c --- QA common block
      data model/'SMERGE      '/

c --- VALID common block
      data ntotal/0/,nvalid/mxss8*0/,ncalms/mxss*0/

      end

c----------------------------------------------------------------------
c --- BRING IN CALPUFF SYSTEM UTILITY SUBROUTINES
      include 'calutils.for'
      include 'presutil.for'
c----------------------------------------------------------------------

c----------------------------------------------------------------------
      subroutine setup
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 020809            SETUP
c                D. Strimaitis, Earth Tech, Inc.
c
c --- PURPOSE:   perform all initialization and one-time setup
c                operations
c
c --- UPDATES:
c     V5.1(020809) from V5.0(010630) (D. Strimaitis)
c     - Use parameter ENVPOP to activate call to RDSSTA
c
c     Common block /FILNAM/
c        RUNINP
c
c --- Parameters used:
c        IO5, IO6, IOMESG, ENVPOP
c
c --- SETUP called by: MAIN
c --- SETUP calls:     DATETM, COMLINE, READCF, WRTHEAD
c                      
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.smg'

c --- Include common blocks
      include 'filnam.smg'
      include 'qa.smg'

c --- Get date and time from system
      call DATETM(rdate,rtime,rcpu)

c --- Get the name of the control file from the command line
      call COMLINE(runinp)

c --- Open the control file
      open(io5,file=runinp,status='old')

c --- Report progress
      write(iomesg,*)'SETUP PHASE'

c --- Check that the version and level number in the parameter
c --- file matches those in the code itself
      if(ver.ne.mver.or.level.ne.mlevel)then
         write(iomesg,10) ver,level,mver,mlevel
10       format(/1x,'ERROR in SUBR. SETUP -- The SMERGE version ',
     1   'and level numbers do not match those in the parameter file'/
     2   5x,'    Model Code - Version: ',a12,' Level: ',a12/
     3   5x,'Parameter File - Version: ',a12,' Level: ',a12)
         stop
      endif

c --- Read control file (open other files)
      call READCF


c --- Read station information file
      if(envpop.EQ.1) call RDSSTA

      return
      end
c----------------------------------------------------------------------
      subroutine readcf
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 121203           READCF
c                D. Strimaitis, Earth Tech, Inc.
c
c --- PURPOSE:  Read the file containing the file names of the
c               input and output files of the run, and associated
c               control variables, place into program variables,
c               and QA information.
c
c --- UPDATES:

c --- Ver 5.7.0 lev 121203 from Ver 5.64 lev 070912
c     - Output version string to list-file as a12

c --- V5.63(070327) to V5.64(070912) (C. Escoffier-Czaja)
c     - Add option for generic format - add checks when JDAT=8 (generic format)
c --- V5.61(060309) to V5.63(070327) (D. Strimaitis)
c     - Loop over stations when writing time zones to list file used
c       wrong index (all zones written as zero)
c --- V5.601(050921) to V5.61(060309) (D. Strimaitis)
c     - Filenames from c*70 to c*132 for CALUTILS V2.3 and later
c --- V5.6 (041123) to V5.60(050921) (F.Robe)
c     - bug fix (typo ibjul/ibjuldy and iejul/iejuldy) so that for beg/ending hour of 24
c       the beg/ending day is incremented when the hour is reset to 0
c     V5.6 (041123) from V5.52(041026) (F.Robe)
c     - New Smerge format. First line has input file version number
c       (SMERGE.INP 2.1)
c     - Computation period defined in SMERGE.INP with explicit  
c       beginning/ending times including seconds 
c     - Time zone :character string (relative to UTC) instead of real
c       (ASTZ instead of XSTZ and ABTZ instead of XBTZ)
c     - Convert those explicit times back to hour-ending times
C       to use internally in the code
c
c     V5.52(041026) from V5.51(040922)
c     - Require time zone XSTZ entered in control file to be 0 for
c       ISHWO, TD3505, and TD9956                   (K. Morrison)
c     - Disable the VSRN output option pending revisions
c                                                   (D. Strimaitis)
c
c     V5.51(040922) from V5.5_draft(040709) (D. Strimaitis)
c     - Add control file variable IHUSWO to designate either English or
c       metric data units in HUSWO file (default=English)
c
c     V5.5_draft(040709) from V5.44(040616) (K. Morrison)
c     - Add VSRN.DAT output option
c     - Add data types 6 & 7 for TD3505 and TD9956
c
c     V5.44(040616) from V5.41(040210) (K. Morrison)
c     - Add variable LPCALC to enable pressure estimation from sea 
c       level values when station pressure is missing for CD144 
c       (Input Group 1).  This variable is forced to be .true. for
c       ISHWO data
c
c     V5.41(040210) from V5.4(040205) (K. Morrison)
c     - Add station elevation for ISHWO processing
c
c     V5.4(040205) from V5.31(020809) (K. Morrison)
c     - Add ISHWO processing (file type 5)
c
c     V5.1(020809) from V5.0(010630) (D. Strimaitis)
c     - Use parameter ENVPOP to control surface station info
c
c     V5.0(020308) from V5.0(010630) (D. Strimaitis)
c     - Add station information file
c     - Use larger integer format when writing station ID
c     - Add JDAT=4 for extended CD144 record
c
c --- INPUTS:
c
c ---    Common block /QA/ variables:
c           VER, LEVEL
c
c        Parameters: IO5, IO6, IOMESG, MXVAR
c                    MXFF, MXSS, ENVPOP
c
c --- OUTPUT:
c
c ---    Common block /DATEHR/ variables:
c           ibdathr,iedathr
c ---    Common block /FILNAM/ variables:
c           prevdat,surfdat,runlst,sfcdat,cffiles,lcfiles
c ---    Common block /CONTROL/ variables:
c           iotz,ioform,iopack,inform,
c           jdat,ihuswo,jtxt,nff,istz(mxff),ifstn(mxff),
c           ibstn(mxss),nbstn,lprev          
c ---    Common block /STATION/ variables:
c           istz(mxff),ifstn(mxff)
c
c --- READCF called by:  SETUP
c --- READCF calls:      READIN, FILCASE, JULDAY, QAYR4, YR4, OPENS
c----------------------------------------------------------------------
c
c --- Include parameter statements and commons
      include 'params.smg'
      include 'params.cal'
c
c --- Include common blocks
      include 'control.smg'
      include 'datehr.smg'
      include 'filnam.smg'
      include 'qa.smg'
      include 'station.smg'
c
c --- Local variables
      real qastz(mxff)
      character*4 ctemp(132,5)
      character*12 cvdic(mxvar,4)
      integer ivleng(mxvar,4),ivtype(mxvar,4)
      logical lecho
      logical lerrcf
      character*44 units(2)
      character*16 inputset,inputver
      character*64 inputmod
      character*70  astz,abtz
      character*8  asftz

c --- Set control file error logical
      lerrcf=.FALSE.

c --- Set strings for reporting HUSWO data units
      data units/'(ENGLISH data units are expected in HUSWO)  ',
     &           '(METRIC data units are expected in HUSWO)   '/

c --- Set Dictionary
      data lecho/.false./
      data names/5/

C --- 4 new dictionary variables (IBSEC,IESEC,ABTZ)
      data cvdic/
     a  'PREVDAT','SURFDAT','RUNLST','SSTATXT', 
     a  'VSRNDAT','LCFILES','NFF',53*' ',
     b  'SFCMET','IFSTN','XSTZ','ASTZ','XELEV', 55*' ',
     c  'IBYR','IBMO','IBDY','IBHR','IBSEC','IEYR','IEMO',
     c  'IEDY','IEHR','IESEC','XBTZ','ABTZ',
     c  'LPREV','NBSTN','INFORM','IOFORM','IOPACK','JDAT','JTXT',
     c  'LPCALC','LVSRNOUT','IHUSWO',38* ' ',
     d  'IBSTN', 59*' '/

      data ivleng/
     a  5*132,2*1, 53*0,
     b  132,2*1,70,1, 55*0,
     c  11*1,70,10*1,38*0,
     d  1, 59*0/

c --- Variable types (ivtype) are coded as:
c          0 = null
c          1 = real
c          2 = integer
c          3 = logical
c          4 = character
      data ivtype/
     a  5*4,3,2, 53*0,
     b  4,2,1,4,1, 55*0,
     c  10*2,1,4,3,6*2,2*3,2, 38*0,
     d  2, 59*0/




c ------------------
c --- Input format 
c -------------------
c --- New format: first line includes dataset types, version number
c --- and description (starts at 2.1)
      read(io5,'(2a16,a64)') inputset,inputver,inputmod
      

c ------------------
c --- Input Group 0a
c ------------------

c --- Initialize the temporary arrays
      do i=1,names
         do j=1,132
            ctemp(j,i)(1:1)=' '
         enddo
      enddo

c --- Read the group data
       call READIN(cvdic(1,1),ivleng(1,1),ivtype(1,1),io5,iomesg,
     & lecho,
     1 ctemp(1,1),ctemp(1,2),ctemp(1,3),ctemp(1,4),ctemp(1,5),
     2 lcfiles,nff,
     3 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     4 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     5 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     6 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     7 idum,idum,idum,idum,idum)

c --- Prepare any filenames included in the I/O file by erasing
c --- the default filename set above
      if(ctemp(1,1)(1:1).ne.' ')prevdat=' '
      if(ctemp(1,2)(1:1).ne.' ')surfdat=' '
      if(ctemp(1,3)(1:1).ne.' ')runlst=' '
      if(ctemp(1,4)(1:1).ne.' ')sstatxt=' '
      if(ctemp(1,5)(1:1).ne.' ')vsrndat=' '

c --- Transfer the char*4 data into the char*132 variables
      do j=1,132
         if(ctemp(j,1)(1:1).ne.' ')prevdat(j:j)=ctemp(j,1)(1:1)
         if(ctemp(j,2)(1:1).ne.' ')surfdat(j:j)=ctemp(j,2)(1:1)
         if(ctemp(j,3)(1:1).ne.' ')runlst(j:j)=ctemp(j,3)(1:1)
         if(ctemp(j,4)(1:1).ne.' ')sstatxt(j:j)=ctemp(j,4)(1:1)
         if(ctemp(j,5)(1:1).ne.' ')vsrndat(j:j)=ctemp(j,5)(1:1)
      enddo

c --- Convert the file names to the proper case
      call FILCASE(lcfiles,prevdat)
      call FILCASE(lcfiles,surfdat)
      call FILCASE(lcfiles,runlst)
      call FILCASE(lcfiles,sstatxt)
      call FILCASE(lcfiles,vsrndat)

c --- Open listfile
      open(io6,file=runlst,status='unknown')

c --- Write banner to list file
      write(io6,5) ver,level

c --- v5.7.0 (121203)
5     format(///,26x,'SMERGE OUTPUT SUMMARY',/,17x,'VERSION:  ',A12,
     1       ' LEVEL:  ',A8///)

c ------------------
c --- Input Group 0b
c ------------------

      do k=1,nff
c ---    Initialize the temporary arrays for the file names and time zones
         do j=1,132
            ctemp(j,1)(1:1)=' '
            ctemp(j,2)(1:1)=' '
            sfcmet(j:j)=' '
         enddo
         do j=1,70
            astz(j:j)=' '
         enddo

c ---    Set default elevation
         xelev=-9999.

c ---    Read the surface met station information
       call READIN(cvdic(1,2),ivleng(1,2),ivtype(1,2),io5,io6,lecho,
     1 ctemp(1,1),ifstnin,xstz,ctemp(1,2),xelev,
     2 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     3 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     4 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     5 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     6 idum,idum,idum,idum,idum,idum,idum)

c ---    Transfer the char*4 data into the char*132 variable
         do j=1,132
            if(ctemp(j,1)(1:1).ne.' ')sfcmet(j:j)=ctemp(j,1)(1:1)
         enddo

c ---    Transfer the char*4 data into the char*70 variable
         do j=1,70
            if(ctemp(j,2)(1:1).ne.' ')astz(j:j)=ctemp(j,2)(1:1)
         enddo

c ---    Convert the file name to the proper case
         call FILCASE(lcfiles,sfcmet)

c ---    Convert station UTC time zone to integer base sf time zone
         if (inputver.EQ.'2.1') then
c ---       Strip charct time zone from blanks and take 8 characters
            ij=0
            do j=1,70
               if (astz(j:j).ne.' ') then
                  ij=ij+1
                  if (ij.gt.8) then
                     write(io6,*)'wrong format for UTC time zone ASTZ'
                     STOP 'wrong format for input UTC time zone - STOP'
                  endif
                  asftz(ij:ij)=astz(j:j)
               endif
            end do
            call utcbasr(asftz,xstz)
         else
             call basrutc(xstz,asftz)
         endif
  
c ---    Place information in surface station arrays, up to MXFF
         if(k.LE.MXFF) then
            cffiles(k)=sfcmet
            ifstn(k)=ifstnin
            istz(k)=NINT(xstz)
            qastz(k)=xstz
            elev(k)=xelev
         endif

      enddo

c -----------------
c --- Input Group 1
c -----------------
C --- Initialize date variable to check whether old or new input file 
      ibsec=-9999
      iesec=-9999
      xbtz=-999.

c --- Initialize the ABTZ array for the UTC time zone
      do j=1,70
         ctemp(j,1)(1:1)=' '
         abtz(j:j)=' '
      enddo

      call readin(cvdic(1,3),ivleng(1,3),ivtype(1,3),io5,io6,lecho,
     1 IBYR,IBMO,IBDY,IBHR,IBSEC,IEYR,IEMO,IEDY,IEHR,IESEC,XBTZ,
     2 CTEMP(1,1),LPREV,NBSTN,INFORM,IOFORM,IOPACK,JDAT,JTXT,LPCALC,
     3 LVSRNOUT,IHUSWO,
     4 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     5 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     6 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     7 idum,idum)

c --- Make sure beg/ending years are YYYY (Y2K)
      call QAYR4(io6,ibyr,0,ierr)
      if(ierr.NE.0) then
         write(io6,*) 'READCF: Y2K Error in Input Group 1'
         lerrcf=.TRUE.
      endif
      call YR4(io6,ieyr,ierr)
      if(ierr.NE.0) then
         write(io6,*) 'READCF: Y2K Error in Input Group 1'
         lerrcf=.TRUE.
      endif

c --- Compute Julian day
      call JULDAY(io6,ibyr,ibmo,ibdy,ibjuldy)
      call JULDAY(io6,ieyr,iemo,iedy,iejuldy)

c --- Make sure hours are between 0-23
      if ((ibhr.gt.24) . or. (iehr.gt.24)) then
         write(io6,*)'IBHR-IEHR must be between 0 and 23'
         stop 'STOP in READCF - IBHR-IEHR must be between 0 and 23'
      else if (ibhr.eq.24) then
         ibhr=0
c ---     bug fix 09/21/2005 ibjuldy instead of ibjul (frr)       
         call INCR(io6,ibyr,ibjuldy,ibhr,24)
         call GRDAY(io6,ibyr,ibjuldy,ibmo,ibdy)
      else if (iehr.eq.24) then
         iehr=0
c ---     bug fix 09/21/2005 iejuldy instead of iejul (frr)       
         call INCR(io6,ieyr,iejuldy,iehr,24)
         call GRDAY(io6,ieyr,iejuldy,iemo,iedy)
      endif

      if (inputver.EQ.'2.1') then
c ---    New input format with beginning/ending times including seconds
c ---    check that new variables IBSEC,IESEC are there

         if( (ibsec.eq.-9999).or.(iesec.eq.-9999) ) then
            write(io6,*)'SMERGE.INP dataset version 2.1'
            write(io6,*)'Requires beginning/ending seconds: IBSEC,IESEC'
            write(io6,*)'Update your input file - STOP'
            STOP 'Error in control file - check LST file -STOP -'
         endif


c ---    Convert seconds to hours
         if(ibsec.GE.3600) then
            nhrinc=ibsec/3600
            ibsec=ibsec-nhrinc*3600
            call INCR(io6,ibyr,ibjuldy,ibhr,nhrinc)
            call GRDAY(io6,ibyr,ibjuldy,ibmo,ibdy)
         endif
         if(iesec.GE.3600) then
            nhrinc=iesec/3600
            iesec=iesec-nhrinc*3600
            call INCR(io6,ieyr,iejuldy,iehr,nhrinc)
            call GRDAY(io6,ieyr,iejuldy,iemo,iedy)
         endif

c ---    output variables in explicit times (list file)
         ibyrn=ibyr
         ibmon=ibmo
         ibdyn=ibdy
         ibhrn=ibhr
         ibsecn=ibsec
         ieyrn=ieyr
         iemon=iemo
         iedyn=iedy
         iehrn=iehr
         iesecn=iesec

c ---    Convert explicit beginning time to hour-ending beg. times
c ---    as hour-ending times are used in the computational part of SMERGE
c ---    Explicit ending time is the same as hour-ending ending time
c ---    because only hourly data are dealt with at this stage (041123)
         call INCR(io6,ibyr,ibjuldy,ibhr,+1)


c ---    New time zone convention; Character string with time relative
c ---    to UTC - AXTZ= "UTC-XBTZ" 
c ---    Transfer the char*4 data into the char*70 variable
         do j=1,70
            if(ctemp(j,1)(1:1).ne.' ')abtz(j:j)=ctemp(j,1)(1:1)
         enddo
c ---    Strip charct time zone from blanks in ABTZ and take 8 characters
c ---    to form AXTZ (shared with other subroutines)
         ij=0
         do j=1,70
            if (abtz(j:j).ne.' ') then
               ij=ij+1
               if (ij.gt.8) then
                  write(io6,*)'wrong format for input UTC time zone'
                  write(io6,*)'ABTZ is char*8: UTC+HHMM '
                  STOP 'wrong format for input UTC time zone - STOP'
               endif
               axtz(ij:ij)=abtz(j:j)
            endif
         end do
         call utcbasr(axtz,xbtz)

      else if ((ibsec.ne.-9999).or.(iesec.ne.-9999)) then
c ---    Mix of old and new format features - Stop
c ---    should be old input file with hour ending times without seconds
         write(io6,*)'SMERGE.INP dataset version prior to 2.1'
         write(io6,*)'For SMERGE.INP version 2.1 add 1st line header'
         write(io6,*)'Update your input file - STOP'
         STOP 'Error in control file - check LST file -STOP - '

      else 
c ---    old input file with hour-ending time spec.
c ---    convert hour-ending times to explicit times for list output purposes only
         ibyrn=ibyr
         ibmon=ibmo
         ibjuln=ibjuldy
         ibhrn=ibhr
         ibsecn=0
         call INCR(io6,ibyrn,ibjuln,ibhrn,-1)
         call GRDAY(io6,ibyrn,ibjuln,ibmonn,ibdyn)

         ieyrn=ieyr
         iemon=iemo
         iedyn=iedy
         iehrn=iehr
         iesecn=0


c ---    Convert real time zone to "UTC+HHMM"
         call basrutc(xbtz,axtz)

      endif


c --- Create date-time parameters (YYYYJJJHH)
      ibdathr = ibyr * 100000 + ibjuldy * 100 + ibhr
      iedathr = ieyr * 100000 + iejuldy * 100 + iehr


c -----------------
c --- Input Group 2
c -----------------

c --- Condition NBSTN to be consistent with LPREV
      if(.not.LPREV) nbstn=0

      do k=1,nbstn
         kk=MIN(k,mxss)
         call readin(cvdic(1,4),ivleng(1,4),ivtype(1,4),io5,io6,lecho,
     1 IBSTN(kk),
     2 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     3 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     4 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     5 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     6 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum)
      enddo

c -------------------------------------------------
c --- Translate selected inputs to SMERGE variables
c -------------------------------------------------
      iotz=NINT(xbtz)

c ---------------------
c --- Perform QA checks
c ---------------------

c --- Test for valid NFF
      if(nff.GT.mxff) then
         write(io6,*)
         write(io6,*) 'READCF:  Error in Input Group 0a'
         write(io6,*) 'NFF exceeds the parameter MXFF '
         write(io6,*) 'NFF, MXFF = ',nff,mxff
         write(io6,*) 'Increase MXFF in PARAMS.SMG and recompile'
         lerrcf=.TRUE.
      endif

c --- Test for integer time zone (code for half-zones not available)
      kff=MIN(nff,mxff)
      do k=1,kff
         test=istz(k)-qastz(k)
         if(ABS(test).GE.0.1) then
            write(io6,*)
            write(io6,*) 'READCF:  Error in Input Group 0b'
            call basrutc(qastz(k),afstz)
            write(io6,*) 'Fractional time zone found: ASTZ= ',afstz
            write(io6,*) 'SMERGE is designed to use integer time zones'
            lerrcf=.TRUE.
         endif
         if(istz(k).LT.-12 .OR. istz(k).GT.12) then
            write(io6,*)
            write(io6,*) 'READCF:  Error in Input Group 0b'
            xbtz=istz(k)
            call basrutc(xbtz,afstz)
            write(io6,*) 'Invalid station time zone = ',afstz
            write(io6,*) ' ( UTC-1200 <= ASTZ <= UTC+1200 ) '
            lerrcf=.TRUE.
         endif
c ---    Test for time zone other than 0 for UTC files
c        (ISHWO, TD3505, TD9956)
         if(JDAT.GE.5 .AND. JDAT.LE.7 .AND. istz(k).NE.0) then
            write(io6,*)
            write(io6,*) 'READCF:  Error in Input Group 0b'
            xbtz=istz(k)
            call basrutc(xbtz,asftz)
            write(io6,*) 'Non-zero time zone found: ASTZ= ',asftz
            write(io6,*)'ISHWO,TD3505,and TD9956 are time zone UTC+0000'
            lerrcf=.TRUE.
         endif
      enddo

c --- Test for Time zone of output data (IOTZ)
c --- Test for integer time zone (code for half-zones not available)
      test=iotz-xbtz
      if(ABS(test).GE.0.1) then
         write(io6,*)
         write(io6,*) 'READCF:  Error in Input Group 1'
         write(io6,*) 'Fractional time zone found: AXTZ= ',axtz
         write(io6,*) 'SMERGE is designed to use integer time zones'
         lerrcf=.TRUE.
      endif
      if(iotz.LT.-12 .OR. iotz.GT.12) then
         write(io6,*)
         write(io6,*) 'READCF:  Error in Input Group 1'
         write(io6,*) 'Invalid value of time zone (ABTZ) = ',axtz
         write(io6,*) ' (UTC-1200 <= TIME ZONE <= UTC+1200) '
         lerrcf=.TRUE.
      endif

c --- Test output data formats
      if(ioform.NE.1 .AND. ioform.NE.2) then
         write(io6,*)
         write(io6,*) 'READCF:  Error in Input Group 1'
         write(io6,*) 'Invalid value of IOFORM = ',ioform
         write(io6,*) 'IOFORM must be 1 or 2'
         lerrcf=.TRUE.
      endif
c --- Packing flag of output SURF.DAT file (0=not packed,1=packed)
      if(iopack.NE.0 .AND. iopack.NE.1) then
         write(io6,*)
         write(io6,*) 'READCF:  Error in Input Group 1'
         write(io6,*) 'Invalid value of IOPACK = ',iopack
         write(io6,*) 'IOPACK must be 0 or 1'
         lerrcf=.TRUE.
      endif

c --- Test for valid JDAT
      if(nff.GT.0 .AND. envpop.EQ.1) then
c --- (CEC - 070912) Add a generic file option - JDAT = 8
c         if(jdat.LT.1 .OR. jdat.GT.7) then
         if(jdat.LT.1 .OR. jdat.GT.8) then
            write(io6,*)
            write(io6,*) 'READCF:  Error in Input Group 1'
            write(io6,*) 'JDAT out of range      = ',jdat
            write(io6,*) 'JDAT should be between 1 and 8'
            lerrcf=.TRUE.
         endif
      elseif(nff.GT.0) then
c         if(jdat.LT.1 .OR. jdat.GT.7 .OR. jdat.EQ.4) then
         if(jdat.LT.1 .OR. jdat.GT.8 .OR. jdat.EQ.4) then
            write(io6,*)
            write(io6,*) 'READCF:  Error in Input Group 1'
            write(io6,*) 'JDAT out of range      = ',jdat
            write(io6,*) 'JDAT should be 1,2,3 or 5,6,7,8'
            lerrcf=.TRUE.
         endif
      endif

c --- Test for valid IHUSWO
      if(jdat.EQ.3) then
         if(ihuswo.LT.1 .OR. ihuswo.GT.2) then
            write(io6,*)
            write(io6,*) 'READCF:  Error in Input Group 1'
            write(io6,*) 'IHUSWO out of range      = ',ihuswo
            write(io6,*) 'IHUSWO should be between 1 and 2'
            lerrcf=.TRUE.
         endif
      endif

c --- Test for valid JTXT
      if(nff.GT.0 .AND. envpop.EQ.1) then
         if(jtxt.LT.1 .OR. jtxt.GT.1) then
            write(io6,*)
            write(io6,*) 'READCF:  Error in Input Group 1'
            write(io6,*) 'JTXT out of range      = ',jtxt
            write(io6,*) 'JTXT should be 1'
            lerrcf=.TRUE.
         endif
      endif

c --- Test for valid NBSTN
      if(nbstn.GT.mxss) then
         write(io6,*)
         write(io6,*) 'READCF:  Error in Input Group 1'
         write(io6,*) 'NBSTN exceeds the parameter MXSS '
         write(io6,*) 'NBSTN, MXSS = ',nbstn,mxss
         write(io6,*) 'Increase MXSS in PARAMS.SMG and recompile'
         lerrcf=.TRUE.
      endif

c --- Deactivate VSRN output option
      if(lvsrnout) then
         write(io6,*)
         write(io6,*) 'READCF:  Error in Input Group 1'
         write(io6,*) 'Unsupported option detected:    LVSRNOUT'
         write(io6,*) 'VSRN output file option is under review'
         write(io6,*) 'Set LVSRNOUT = F in the control file'
         lerrcf=.TRUE.
      endif

c --- STOP now if error exists in the control file
      if(LERRCF) then
         write(*,*)'ERRORS are found in the CONTROL file'
         write(*,*)'Review messages written to the LIST file'
         stop
      endif

c ------------------------
c --- Open remaining files
c ------------------------
      call OPENS

c -----------------------------------------------------------
c --- Echo inputs to list file as in previous SMERGE versions
c -----------------------------------------------------------

c --- Echo filenames
      write(io6,*)
      write(io6,*) 'Control file name: ',runinp
      write(io6,*) 'Output list file name: ',runlst
      write(io6,*) 'Output file name: ',surfdat
      if(envpop.EQ.1) write(io6,*) 'Station Info file name: ',sstatxt
      write(io6,*) 'Continuation Run? ',lprev
      if(LPREV)
     1  write(io6,*) 'Previous SMERGE output data file: ', prevdat
      write(io6,*)
      if(LVSRNOUT) write(io6,*) 'Weather-visibility file: ',vsrndat

      if (jdat.EQ.1) write(io6,20)
      if (jdat.EQ.2) write(io6,21)
      if (jdat.EQ.3) write(io6,22)
      if (jdat.EQ.4 .AND. envpop.EQ.1) write(io6,23)
      if (jdat.EQ.5) write(io6,24)
      if (jdat.EQ.8) write(io6,26)
20    format(//1x,'Station ID',4x,'Time Zone ',2x,'Formatted CD144'
     1      ,' Surface Data  ',/35x,'Input Files ',/)
21    format(//1x,'Station ID',4x,'Time Zone ',2x,'SAMSON Surface',
     1       ' Data  ',/35x,'Input Files ',/)
22    format(//1x,'Station ID',4x,'Time Zone ',2x,'HUSWO Surface',
     1       ' Data  ',/35x,'Input Files ',/)
23    format(//1x,'Station ID',4x,'Time Zone ',2x,'Extended CD144'
     1      ,' Surface Data  ',/35x,'Input Files ',/)
24    format(//1x,'Station ID',4x,'Time Zone ',5x,'ISHWO Surface',
     1       ' Data  ',/35x,'Input Files ',/)
26    format(//1x,'Station ID',4x,'Time Zone ',5x,'Generic Format',
     1       ' Data  ',/35x,'Input Files ',/)

      do i=1,nff
         xbtz=istz(i)
         call basrutc(xbtz,asftz)
         write(io6,'(1x,i10,2x,a8,4x,a)') ifstn(i),asftz,cffiles(i)
      enddo

c --- Identify units in HUSWO file
      if (jdat.EQ.3) then
         write(io6,*)
         write(io6,*) units(ihuswo)
         write(io6,*)
         write(*,*) units(ihuswo)
      endif

c --- If JDAT=5 to 7 (ISHWO, 3505, 9956), make sure LPCALC enabled
      if(jdat.ge.5.and.jdat.le.7.and..not.lpcalc) then
         lpcalc=.true.
         write(io6,*)
         write(io6,*) 'READCF:  Note in Input Group 1'
         write(io6,*) 'LPCALC reset to .true. for integrated data'
      endif

c --- Echo processing period
      write(io6,80) axtz,ibmon,ibdyn,ibyrn,ibhrn,ibsecn,
     :                   iemon,iedyn,ieyrn,iehrn,iesecn
80    format(//,1x,'Period to Extract (in time zone:',a8,'):   ',i2,'/',
     1  i2,'/',i4,2x,i2,':',i4,'  to  ',i2,'/',i2,'/',i4,2x,i2,':',i4/)


      return
      end


c----------------------------------------------------------------------
      subroutine comp
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 080229             COMP
c                E. Insley, J. Scire,   Earth Tech, Inc.
c
c --- PURPOSE:  Process the surface station data to produce a
c               SURF.DAT file for CALMET
c
c --- UPDATES:
c     V5.65(080229) from V5.64(070912 (C. Escoffier-Czaja)
c     - Add checking of units in header record of generic format
c     - GENERIC format was upgraded to version 2.0 
c       (a third line is added in header)
c     V5.64(070912) from V5.52(041026) (C. Escoffier-Czaja)
c     - Add reading header for generic format file
c     V5.52(041026) from V5.3(030402) (D. Strimaitis)
c     - Added station ID check (the ID provided in the control file for
c       each station file processed must match the ID found in the
c       corresponding file.
c     V5.3(030402) from V5.0(020308) (D. Strimaitis)
c     - Move previous SURF.DAT header output to RDHD
c     V5.0(020308) from V5.0(010630) (D. Strimaitis)
c     - Use large integer format when writing station ID instead of 
c       the char*5 variable
c
c --- INPUTS:
c
c        Parameters: IO6, IOFF, IOMESG, MXFF, MXSS
c
c --- OUTPUT:
c           none
c
c --- COMP called by:  MAIN
c --- COMP calls:      GRDAY, DEDAT,
c                      RDWRITS, CHKHUSHD
c ------------------------------------------------------------------
c
c --- Include parameter statements and commons
      include 'params.smg'

c --- Include common blocks
      include 'control.smg'
      include 'datehr.smg'
      include 'filnam.smg'
      include 'qa.smg'
      include 'station.smg'

c --- Define common for unused variables
      common/skip/jd,ctext

      integer ishift(3)
      character*80 ctext
      character*120 hushd
c
c --- Header Generic file
      integer ihead_err
      character*5 amon
      character*3 aday,awd
      character*4 ahr,atemp,aws,ayr
      character*2 aprec,apres
      character*1 arh
      character*6 acov
      character*16 achgt
      character*7 frmt_gen,ctxtgen



c --- Report progress
      write(iomesg,*)'COMPUTATIONAL PHASE'

c --- Process the two header records in the SAMSON files
      if(jdat.eq.2)then
         io=ioff
         do i=1,nff
            read(io,'(1x,i5)')jd
            read(io,'(a80)')ctext
            io=io+1
c ---       Check the ID
            if(jd.NE.ifstn(i)) then
               write(*,*)'FATAL Error in COMP -- Station ID bad'
               write(io6,*)'FATAL Error in COMP -- Station ID bad'
               write(io6,*)'File number (SAMSON): ',i
               write(io6,*)' Expected Station ID: ',ifstn(i)
               write(io6,*)'    Found Station ID: ',jd
               stop
            endif
         enddo
c --- Skip the one header record in the HUSWO files
      elseif(jdat.eq.3)then
         io=ioff
         do i=1,nff
            read(io,'(a120)')hushd
            call chkhushd(io6,hushd)
            io=io+1
         enddo
c --- Process the header records in the GENERIC files
      elseif(jdat.eq.8)then
         io=ioff
         do i=1,nff
            ihead_err=0
            read(io,'(a7)')frmt_gen
            if(frmt_gen.ne.'GENERIC') then
            write(*,*)'ERROR - if this is a generic format'
            write(*,*)'the header should start with the word'
            write(*,*)'GENERIC'
            stop
            endif
c
            read(io,*)ctxtgen,ctxtgen,ctxtgen,jd
c 
            read(io,*)amon,aday,ayr,ahr,atemp,aprec,apres,arh,awd,
     1           aws,acov,achgt
            if (amon.ne.'month'.and.amon.ne.'Month') ihead_err=1
            if (aday.ne.'day'.and.aday.ne.'Day') ihead_err=1
            if (ayr.ne.'year'.and.ayr.ne.'Year') ihead_err=1
            if (ahr.ne.'hour'.and.ahr.ne.'Hour') ihead_err=1
            if (atemp.ne.'degC'.and.atemp.ne.'DegC') ihead_err=1
            if (aprec.ne.'mm') ihead_err=1
            if (apres.ne.'mb') ihead_err=1
            if (arh.ne.'%') ihead_err=1
            if (awd.ne.'deg') ihead_err=1
            if (aws.ne.'ms-1') ihead_err=1
            if (acov.ne.'tenths') ihead_err=1
            if (achgt.ne.'hundreds_of_feet') ihead_err=1
            if (ihead_err.eq.1) then
            write(*,*)'ERROR in Generic format station header'
            write(*,*)'Data units provided need to be checked'
            write(*,*)'that they are in the required units'
            write(*,*)'Note also: Comma delimited format'
            write(*,*)'Format for header required is the following:'
         write(*,*)'month,day,year,hour,degC,mm,mb,%,deg,ms-1'
         write(*,*)'tenths,hundreds_of_feet'
            write(*,*)' Units read:'
         write(*,*)amon,',',aday,',',ayr,',',ahr,',',atemp,',',aprec
         write(*,*)apres,',',arh,',',awd,',',aws,',',acov,',',achgt
         write(*,*)
            stop
            endif
            io=io+1
c ---       Check the ID
            if(jd.NE.ifstn(i)) then
               write(*,*)'FATAL Error in COMP -- Station ID bad'
               write(io6,*)'FATAL Error in COMP -- Station ID bad'
               if(jdat.eq.8) then
               write(io6,*)'File number (GENERIC): ',i
               endif
               write(io6,*)' Expected Station ID: ',ifstn(i)
               write(io6,*)'    Found Station ID: ',jd
               stop
            endif
         enddo
      endif
c
C  READ DATA AND WRITE TO OUTPUT FILE
      call rdwrits

      write(io6,81)
81    format(//,26x,'********************',//)

C  WRITE OUT SUMMARY INFORMATION ABOUT OUTPUT FILE
      if(ioform.EQ.1)then
         write(io6,135) axtz, ioform, iopack
135      format(/,1x,'Characteristics of SMERGE Output ',
     1         'Data File:'//3x,
     2         'Time Zone:',a8/3x,'File Format (1=unformatted, ',
     3         '2=formatted):',i3/3x,'Packing Code:',i3)
      elseif(ioform.EQ.2)then
         write(io6,136) iotz, ioform
136      format(/,1x,'Characteristics of SMERGE Output ',
     1         '(SURF.DAT) File:'//3x,
     2         'Time Zone:',i6/3x,'File Format (1=unformatted, ',
     3         '2=formatted):',i3)
      else
         write(io6,137) ioform
137      format(1x,'ERROR-- Invalid input for IOFORM (1 or 2); ',
     1         'IOFORM = ',i3)
         write(*,987)
         goto 99
      endif


C     WRITE TO LIST FILE THE STATION NUMBERS IN THE OUTPUT FILE
C     SET VARIABLES FOR PROPER COLUMNS FOR WRITING OUT
C      J4 IS NO. ROWS IN A "SHORT" COLUMN
C      J5 IS NO. ROWS IN A "LONG" COLUMN
C      J6 IS THE NUMBER OF "LONG" COLUMNS
      j4 = ntstn/4
      j6 = mod(ntstn,4)
      if(j6.EQ.0)then
        j5 = j4
      else
        j5 = j4 + 1
      endif
      ishift(1) = j5
      do i=2,3
        if(i.LE.j6)then
          ishift(i) = ishift(i-1) + j5
        else
          ishift(i) = ishift(i-1) + j4
        endif
      enddo
      ncol = min0(ntstn,4)
      write(io6,150) (' ',k=1,ncol)
150   format( /,1x,'Surface Stations in Output File:  '/
     1       3x,4(a1,'No.',7x,'ID',8x)/)
      do i=1,j4
        i2 = i + ishift(1)
        i3 = i + ishift(2)
        i4 = i + ishift(3)
        write(io6,160) i,idstn(i),i2,idstn(i2),i3,idstn(i3),
     &                 i4,idstn(i4)
160     format(3x,4(i3,2x,I10,6x))
      enddo
      if(j6.GT.0)then
        n1 = j5
        n2 = n1+(j6-1)*j5
        write(io6,160) (k,idstn(k),k=n1,n2,j5)
      endif

      write(io6,*)
      write(io6,*)
      write(io6,*)

      return

99    stop
987   format(1x,'ERROR in SMERGE run - see SMERGE.LST file')

      end
c----------------------------------------------------------------------
      subroutine opens
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 040709            OPENS
c ---            E. Insley, Earth Tech, Inc.

c
c     Opens input and output data files
c
c --- UPDATES:
c     V5.5_draft(040709) from V5.1(020809) (K. Morrison)
c     - Open VSRN.DAT if used
c     V5.1(020809) from V5.0(020308) (D. Strimaitis)
c     - Use parameter ENVPOP to activate SSTA file
c
c --- OPENS called by:  READCF
c --- OPENS calls:      none
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.smg'
c --- Include common blocks
      include 'control.smg'
      include 'filnam.smg'

C     OPEN FORMATTED INPUT FILES
      io = ioff
      do i=1,nff
         open(io,file=cffiles(i),status='old')
         io = io + 1
      enddo

C     OPEN EXISTING SURF.DAT INPUT FILE
      if(LPREV)then
         if(inform.EQ.1)then
c ---       BINARY INPUT FILE
            open(ioprev,file=prevdat,status='old',form='unformatted')
         elseif(inform.EQ.2)then
c ---       FORMATTED INPUT FILE
            open(ioprev,file=prevdat,status='old')
         endif
      endif

c --- Open the station information file
      if(envpop.EQ.1) open(iossta,file=sstatxt,status='old')

c     OPEN OUTPUT SURF.DAT FILE
      if(ioform.EQ.1)then
c ---    BINARY OUTPUT FILE
         open(iosurf,file=surfdat,status='unknown',form='unformatted')
      elseif(ioform.EQ.2)then
c ---    FORMATTED OUTPUT FILE
         open(iosurf,file=surfdat,status='unknown')
      endif

c     OPEN OUTPUT PRECIP.DAT FILE (name is fixed)
      if(jdat.EQ.4 .AND. envpop.EQ.1)then
c ---    FORMATTED OUTPUT FILE
         open(ioprec,file='precip.dat',status='unknown')
      endif

c     Open the VSRN.DAT file
      if(lvsrnout) then
c ---    if a continuation run, position to end of file
         if(lprev) then
            open(iovsrn,file=vsrndat,position='append')
         else
            open(iovsrn,file=vsrndat)
         endif
      endif

      return
      end
c----------------------------------------------------------------------
      subroutine rdwrits
c----------------------------------------------------------------------
c     
c --- SMERGE     Version: 5.7.0     Level: 100713          RDWRITS
c ---            E. Insley, Earth Tech, Inc.
c
c --- Updated v5.66 (100713) from V5.652 (070912)  (J. Scire)
c     - Add table of valid data by variable and station with
c       hours of valid data and percentage.  Also include hours
c       and percentage of calm winds at each station
c     - Clarify wording of existing table of misisng data
c       (count when all stations are missing)
c
c --- Updated V5.64(070912) from V5.6(041123) (C.Escoffier-Czaja)
c     - Add the possibility to read a generic format meteorological
c       station (JDAT=8)
c
c --- Updated V5.6(041123) from V5.54(041102) (F.Robe)
c     - Include data version in calling list to RDHD and RDS
c     - IPWOUT/ISAMPW array assignement replaced by explicit components
c       assignment
c
c --- Updated V5.54(041102) from V5.53(041029)
c     - Fixed bug in RDWRITS that assigned the wrong station
c       elevation array element when adding stations to an
c       existing SURF.DAT file.                    (D. Strimaitis)
c
c --- Updated V5.53(041029) from V5.52(041026)
c     - Modified End-of-File response after call to GETISH to continue
c       processing valid data on the last record of a file.  Missing
c       station pressure on the last record was not calculated in 
c       previous version (JDAT=5,6,7).             (D. Strimaitis)
c
c --- Updated V5.52(041026) from V5.51(040922) (D. Strimaitis)
c     - Added station ID check (the ID provided in the control file for
c       each station file processed must match the ID found in the
c       corresponding file.
c     - Fixed bug in call to GETISH in RDWRITS that used the wrong
c       station time zone array element when adding stations to an
c       existing SURF.DAT file.                     (K. Morrison)
c --- Updated V5.51(040922) from V5.5_draft(040709) (K. Morrison)
c     - Correction to empirical calculations for pressure
c     - Remove substitution of total cloud cover for opaque for
c       CD144 if opaque is missing
c     - Correct reference to station id for SAMSON and HUSWO
c     - Correct use of present weather for precipitation codes for
c       SAMSON (undo an error)
c     - Correct identification of missing precipitation codes for
c       SAMSON
c     - Correct identification of missing visibility values for
c       SAMSON and HUSWO
c     - Use pressure value to identify HUSWO as English or metric
c       units and STOP if units are not those expected.
c     - Use temperature value to identify HUSWO as English or metric
c       units and STOP if units are not those expected.          (DGS)
c --- Updated V5.5_draft(040709) from V5.44(040621) (K. Morrison)
c     - Add TD3505 and TD9956 file types, and for these types
c       override elevation if valid value read from data file
c     - Handle present weather codes and visibility, and output to
c       VSRN.DAT if needed
c --- Updated V5.44(040621) from V5.42(040318) (K. Morrison)
c     - Read dew-point for all file types, and sea-level pressure
c       for CD-144
c     - Add ability to calculate surface pressure from sea-level
c       pressure for CD-144 via either empirical relation or
c       call to SLP2STP
c     - Get altimeter setting and sea-level pressure from GETISH
c       for ISHWO files
c     - Move pressure calculations for ISHWO to here from READISH,
c       allowing empirical relations or by calling subroutines ALP2STP 
c       and SLP2STP
c     - Enable calculation of RH if dry-bulb and dew-point are 
c       present for all data types
c
c --- Updated V5.42(040318) from V5.41(040210) (K. Morrison)
c     - Enforce CD-144 0 WD with non-0 WS to both be missing
c
c --- Updated V5.41(040210) from V5.4(040205) (K. Morrison)
c     - Add station elevation to ISHWO call to permit station pressure
c       calculation based on altimeter setting or sea level pressure 
c       if station pressure missing
c
c --- Updated V5.4(040205) from V5.3(030402) (K. Morrison)
c     - Add ISHWO file type
c
c --- Updated V5.3(030402) from V5.2(020809) (D. Strimaitis)
c     - Drop parameter ENVPOP from RDHD
c
c --- Updated V5.1(020809) from V5.0(020308) (D. Strimaitis)
c     - Pass parameter ENVPOP to header subroutines
c
c --- Updated V5.0(020308) from V5.0(010630) (D. Strimaitis)
c     - Initialize hourly data to missing before reading file
c     - Introduce station location information arrays
c     - Add JDAT=4 for extended CD144 record (precip rate)
c
c --- Updated V5.0(010630) from V4.0(010315) (D. Strimaitis)
c     - Place parameters and commons into include files
c     - NBSTN .LE. 0 triggers use of all stations in previous file
c
c --- Updated 010315 from 991223 (D. Strimaitis)
c     - Y2K treatment of year enforced
c
c --- Updated 991223 from 970718 (Benjamin de Foy)
c     - Add HUSWO format
c
c
C     READS INPUT FILES AND FILLS THE OUTPUT ARRAYS AND WRITES TO
C     THE OUTPUT FILE
c     
c --- DATE VARIABLES:
c     IBDATHR  - integer  - Beginning date of requested time period
c                           in output time zone YYYYJJJHH
c     IEDATHR  - integer  - Ending date of requested time period in
c                           output time zone YYYYJJJHH
c     KBTZ     - integer  - Time zone of existing sfc input data
c     NDATE    - integer  - Time currently wanted in output time zone
c                           YYYYJJJHH
c     KDATE    - integer  - Time currently wanted in existing sfc time
c                           zone YYYYJJJHH
c     JDATE    - integer  - Time currently wanted in output time zone
c                           for a particular formatted station YYYYJJJHH
c     KBDATHR  - integer  - Beginning date of available existing sfc
c                           data in output time zone YYYYJJJHH
c     KEDATHR  - integer  - Ending date of available existing sfc data
c                           in output time zone YYYYJJJHH
c     IKBDATHR - integer  - Beginning date of available existing sfc
c                           data in existing sfc input time zone
c                           YYYYJJJHH
c     IKEDATHR - integer  - Ending date of available existing sfc data
c                           YYYYJJJHH
c     IDATHR   - integer  - Time of record actually read in existing
c                           sfc time zone YYYYJJJHH
c     
c --- OTHER VARIABLES AND FLAGS:
c     IBIN     - integer  - Flag indicating date status of existing sfc
c                           input file vs requested time period;
c                           IBIN = 0  no data in existing sfc input
c                           file for requested time period
c                           IBIN = 1  existing sfc input file contains
c                           data for requested time period
c     
C     RDWRITS called by: COMP
C     RDWRITS calls: DEDAT, DELTT, RDHD, WRHD, WRHDP, RDS,
C                    PCODES, PCODES2, CLOUDS, WRS, INDECR, JULDAY, YR4,
c                    GETISH, ALP2STP, SLP2STP, SCAN144, SCANISHWO
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.smg'
c --- Include common blocks
      include 'control.smg'
      include 'datehr.smg'
      include 'qa.smg'
      include 'station.smg'
      include 'valid.smg'

      integer ixref(mxss),iceil(mxss),icc(mxss),irh(mxss),
     1     ipcode(mxss),ibuf(3,mxss),ixceil(mxss),ixcc(mxss),
     2     ixrh(mxss),ixpcode(mxss),jprecip(5)

c --- Arrays for empirical pressure calculations
      real xintercep(mxss)/mxss*-9999./
      real xslope(mxss)/mxss*-9999./
      real altrat(mxss)/mxss*-9999./
      logical lfirstrd(mxss)/mxss*.true./

c --- Array for counting missing hours for each variable in surf.dat:
      integer ncbdf(7)

c --- Flag for printing missing:
      logical lfprint

c --- Logical to identify extended CD144 format
      logical lx144

c --- Logical for HUSWO as metric
      logical lhmetric

c --- HUSWO: total and opaque sky cover:
      integer itskc,ioskc     
c --- HUSWO: present weather code:
      character*8 pwth
c --- HUSWO: precipitation flag:
      character*1 precfl
c --- HUSWO: snow fall:
      integer isnow
c --- HUSWO: count hours with multiple weather events:
      integer npwth

c --- Conversion inches Hg to milliBars:
      real inHgtomb             
      parameter(inHgtomb = 33.864)

      real xws(mxss),xwd(mxss),xtempk(mxss),xpres(mxss),ws(mxss),
     1     wd(mxss),tempk(mxss),pres(mxss)

c --- (CEC 070912) - for generic format
      real xcwd,xcws,xcpres,xctemp,xcrh,xcceil,xccc,xcprecip

c --- Add array for precipitation rate in mm/hr (extended CD144 format)
      real xprate(mxss)

      character*1 cceil(3),tcc,ccc,jcover1
      character*8 cprecip
      character*16 datavers
      character cwd*2, cws*2, cpres*4, ctemp*3, crh*3, jceilc*7,
     * cslp*4, cdewp*3, cvis*3

c --- Logicals for ISHWO

      logical licc/.true./,leof

c --- Add data for present weather code conversions

      integer*2 hus2dats(10:99)
      integer*2 sam2dats(0:9,9)
      data sam2dats/
     1 95,99,19,18,18,18,19,19,19,999,
     2 61,63,65,80,81,81,66,67,67,999,
     3 60,62,64,51,53,55,56,57,57,999,
     4 71,73,75,87,88,88,78,78,78,999,
     5 85,87,87,70,72,74,77,77,77,999,
     6 79,79,79,89,90,90,87,88,88,999,
     7 45,45,44,31,31,45,49,45,44,999,
     8 4,5,4,31,38,999,31,4,999,999,
     9 79,79,79,999,999,999,999,999,999,999/
      equivalence (hus2dats,sam2dats)
      integer*1 isampw(9)
      integer*2 ipwout(3)
c
c --- Set misisng value indicators for valid data counter
      xmiss=9999.
      imiss=9999
c --- Set real value slightly lower bto account for round-off error
      xmissm=xmiss-0.01

c --- Set flag for extended CD144 format
      lx144=.FALSE.
      if(jdat.EQ.4) lx144=.TRUE.
    
c --- Initialize ncbdf,lfprint: - bdf
      lfprint = .true.
      do j = 1,7
         ncbdf(j) = 0
      enddo
      npwth = 0
c     
C     SET FLAG FOR READING BINARY DATA
      ibin = 1
C     DETERMINE THE NUMBER OF HOURS BETWEEN THE BEGINNING AND
C     ENDING DATES OF OUTPUT FILE
      call dedat(ibdathr,ibyr,ibjul,ibhr)
      call dedat(iedathr,ieyr,iejul,iehr)
      call deltt(ibyr,ibjul,ibhr,ieyr,iejul,iehr,idiff)
C     NREC IS THE NUMBER OF DATA RECORDS IN THE OUTPUT FILE
      nrec = idiff + 1
C     READ HEADER INFORMATION FOR EXISTING SURFACE DATA INPUT FILE
      if(LPREV) then
         call rdhd(io6,inform,ioprev,kbyr,kbjul,kbhr,
     1        keyr,kejul,kehr,kbtz,nstnb,ispack,datavers,
     2        mxss,idbstn,banem,cbname,cblat,cblon)
         ikbdathr = kbyr * 100000 + kbjul * 100 + kbhr
         ikedathr = keyr * 100000 + kejul * 100 + kehr
C     SET UP CROSS REFERENCE OF STATION IDS WITH THEIR CORRESPONDING
C     POSITION IN THE EXISTING SURFACE DATA FILE ARRAY
         if(nbstn.LE.0) then
            nbstn = nstnb
            do j=1,nstnb
               ixref(j) = j
               ibstn(j) = idbstn(j)
            enddo
            go to 16
         endif
         do 5 i = 1,nbstn
            do 15 k = 1,nstnb
               if(ibstn(i).EQ.idbstn(k))then
                  ixref(i) = k
                  go to 5
               endif
 15         continue
 5       continue
C     CONVERT TO IOTZ (OUTPUT) TIME ZONE
 16      if(iotz.EQ.kbtz)then
            kbdathr = ikbdathr
            kedathr = ikedathr
         else
C     CONVERT DATE/HR TO OUTPUT TIME ZONE
            idtz = kbtz - iotz
            call indecr(io6,kbyr,kbjul,kbhr,idtz,0,23)
            call indecr(io6,keyr,kejul,kehr,idtz,0,23)
            kbdathr = kbyr * 100000 + kbjul * 100 + kbhr
            kedathr = keyr * 100000 + kejul * 100 + kehr
         endif
         if(iedathr.LT. kbdathr .OR. ibdathr.GT.kedathr)then
            write(io6,100)
 100        format('  WARNING: The existing surface data input ',
     1           'file has no data for the time period of this run.')
            ibin = 0
         endif
      else
         nbstn = 0
         kbtz = iotz
c        Write header to VSRN.DAT
         if(lvsrnout) write(iovsrn,*) 'STN---'
      endif
C     FILL STATION ARRAYS FOR OUTPUT FILE
      ntstn = nff + nbstn
C     EXISTING SURFACE STATIONS
      do 25 i = 1,nbstn
         idstn(i) = ibstn(i)
         anem(i)  = banem(i)
         cname(i) = cbname(i)
         clat(i)  = cblat(i)
         clon(i)  = cblon(i)
 25   continue
C     FORMATTED STATIONS
      do 35 i = nbstn+1,ntstn
         idstn(i) = ifstn(i-nbstn)
         anem(i)  = fanem(i-nbstn)
         cname(i) = cfname(i-nbstn)
         clat(i)  = cflat(i-nbstn)
         clon(i)  = cflon(i-nbstn)
 35   continue
c     --- Check for repeating station IDs
      do 360 j=1,ntstn
         icheck = idstn(j)
         icnt = 0
         do 370 k=1,ntstn
c     ---     Keep count of matches (it should only match once)
            if(idstn(k).EQ.icheck) icnt = icnt + 1
 370     continue
         if(icnt.GT.1)then
            write(io6,*) ' ERROR: Duplicate Station ID. ID = ',icheck
            write(*,*) ' ERROR: Invalid Input --- See Run LIST file'
            stop
         endif
 360  continue
C     WRITE OUT THE HEADER TO THE SURFACE DATA OUTPUT FILE


      call wrhd(envpop,io6,ioform,iosurf,ibyr,ibjul,ibhr,
     1          ieyr,iejul,iehr,axtz,ntstn,iopack,idstn,anem,
     2          cname,clat,clon,ver,level)

      if(envpop.EQ.1) then
c ---    Write header to PRECIP.DAT file
         if(LX144) call wrhdp(envpop,ioprec,ibyr,ibjul,ibhr,
     1                        ieyr,iejul,iehr,axtz,ntstn,idstn,
     2                        cname,clat,clon,ver,level)
      endif


C     FIRST DATE TO WRITE TO THE OUTPUT FILE
      ndate = ibyr * 100000 + ibjul * 100 + ibhr
      if(iotz.EQ.kbtz)then
         kdate = ndate
      else
C     KDATE IS TIME IN "KBTZ" TIME ZONE WHICH IS EQUIVALENT TO NDATE IN
C     "IOTZ" TIME ZONE
         idtz = iotz - kbtz
         call dedat(ndate,jyr,jjul,jhr)
         call indecr(io6,jyr,jjul,jhr,idtz,0,23)
         kdate = jyr * 100000 + jjul * 100 + jhr
      endif
C     SKIP TO FIRST DATE IN EXISTING SURFACE DATA INPUT FILE IF THERE
C     ARE DATA FOR THE REQUESTED PERIOD OF THIS RUN
      if(ibdathr.GT.kbdathr)then
         if(LPREV .AND. ibin.NE.0)then
            call deltt(kbyr,kbjul,kbhr,ibyr,ibjul,ibhr,iskip)
            do i = 1,iskip
               if (datavers.eq.'2.1') then
c ---               previous SURF.DAT with explicit beg/ending time
c ---               with seconds format
                    call rdsn(io6,inform,nstnb,ispack,ioprev,1,ibuf,
     1              myr,mjul,mhr,ws,wd,iceil,icc,tempk,irh,pres,ipcode)
               else
c ---               SURF.DAT with hour-ending time format
                    call rds(io6,inform,nstnb,ispack,ioprev,1,ibuf,
     1              myr,mjul,mhr,ws,wd,iceil,icc,tempk,irh,pres,ipcode)
               endif
            enddo
         endif
      endif

c ----------------------
C     MAIN LOOP: (HOURS)
c ----------------------

      do 55 i = 1,nrec

C     READ DATA FOR 1 HOUR FROM EXISTING SURFACE DATA INPUT FILE
c     ----------------------------------------------------------
         if(LPREV .AND. ibin.NE.0)then
            if(ndate.LE.kedathr.AND.ndate.GE.kbdathr)then
               if (datavers.eq.'2.1') then
c ---              previous SURF.DAT with explicit beg/ending time
c ---              with seconds format
                   call rdsn(io6,inform,nstnb,ispack,ioprev,0,ibuf,
     1              iyr,ijul,ihr,ws,wd,iceil,icc,tempk,irh,pres,ipcode)
               else
c ---              previous SURF.DAT with hour-ending time format
                   call rds(io6,inform,nstnb,ispack,ioprev,0,ibuf,
     1              iyr,ijul,ihr,ws,wd,iceil,icc,tempk,irh,pres,ipcode)
               endif
 
               idathr = iyr * 100000 + ijul * 100 + ihr

               if(kdate.NE.idathr) then
                  write(io6,200) idathr, kdate
 200  format(1x,'Error in Subr. RDWRITS: Next date and hour ',
     1       'from the existing surface data input file, ',i10/1x,
     2       'does not match the expected date and hour, ',i10)
                  write(*,987)
 987  format(1x,'ERROR in SMERGE run - see SMERGE.LST file')
              STOP 'ERROR in SMERGE run - see SMERGE.LST file -STOP'
                  go to 99
               endif
C     TRANSFER STATIONS OF INTEREST FROM READ BUFFER TO OUTPUT ARRAY
               do 65 j = 1,nbstn
                  index = ixref(j)
                  xws(j) = ws(index)
                  xwd(j) = wd(index)
                  ixceil(j) = iceil(index)
                  ixcc(j) = icc(index)
                  xtempk(j) = tempk(index)
                  ixrh(j) = irh(index)
                  xpres(j) = pres(index)
                  ixpcode(j) = ipcode(index)
                  xprate(j) = 9999.
 65            continue
            else
               do 75 j = 1,nbstn
                  xws(j) = 9999.
                  xwd(j) = 9999.
                  ixceil(j) = 9999
                  ixcc(j) = 9999
                  xtempk(j) = 9999.
                  ixrh(j) = 9999
                  xpres(j) = 9999.
                  ixpcode(j) = 9999
                  xprate(j) = 9999.
 75            continue
            endif
         else
            do 85 j = 1,nbstn
               xws(j) = 9999.
               xwd(j) = 9999.
               ixceil(j) = 9999
               ixcc(j) = 9999
               xtempk(j) = 9999.
               ixrh(j) = 9999
               xpres(j) = 9999.
               ixpcode(j) = 9999
               xprate(j) = 9999.
 85         continue
         endif
C     SET ARRAY INDEX COUNTER FOR FORMATTED STATIONS
         index = nbstn

C     LOOP OVER FORMATTED FILES (STATIONS)
c     ------------------------------------
          do 95 k = 1,nff
            index = index + 1
            iofor = k + (ioff-1)

c ---       Initialize data to missing
            xws(index) = 9999.
            xwd(index) = 9999.
            ixceil(index) = 9999
            ixcc(index) = 9999
            xtempk(index) = 9999.
            ixrh(index) = 9999
            xpres(index) = 9999.
            ixpcode(index) = 9999
            xprate(index) = 9999.
c           ipwout=999
            ipwout(1)=999
            ipwout(2)=999
            ipwout(3)=999
            visb=99999.

C     READ A RECORD OF FORMATTED DATA
            if(iotz.EQ.istz(k))then
               jdate = ndate
            else
C     CORRECT REQUESTED LOCAL TIME TO OUTPUT TIME ZONE
               idtz = iotz - istz(k)
               call dedat(ndate,jyr,jjul,jhr)
               call indecr(io6,jyr,jjul,jhr,idtz,0,23)
               jdate = jyr * 100000 + jjul * 100 + jhr
            endif
c ------
c CD144:
c ------
 17   if (jdat.eq.1 .OR. jdat.eq.4) then
c        Check if pressure calculation was selected, and attempt empirical
c          calculations
         if(LPCALC.and.LFIRSTRD(index)) then
            lfirstrd(index)=.false.
            call scan144(iofor,xintercep(index),xslope(index))
         endif
         if(LX144 .AND. envpop.EQ.1) then
            read(iofor,10,end=95) id,jyr,jmo,jday,jhr,cceil,cvis,
     1                            cprecip,cslp,cdewp,cwd,cws,cpres,
     2                            ctemp,crh,tcc,ccc,pmmhr
         else
            read(iofor,10,end=95) id,jyr,jmo,jday,jhr,cceil,cvis,
     1                            cprecip,cslp,cdewp,cwd,cws,cpres,
     2                            ctemp,crh,tcc,ccc
         endif
c 10      format(i5,4i2,3a1,4x,a3,a6,9x,2a2,a4,a3,3x,a3,a1,22x,a1)
 10      format(i5,4i2,3a1,4x,a3,a8,a4,a3,2a2,a4,a3,3x,a3,a1,
     *          22x,a1,f3.0)

          ichkws = 0

c ---    Make sure year is YYYY (Y2K)

         call YR4(io6,jyr,ierr)


         if(ierr.NE.0) stop 'Halted in RDWRITS - see list file'
C     CONVERT DATE READ FROM FORMATTED FILE AND COMPARE TO DATE WANTED
         call julday(io6,jyr,jmo,jday,jjul)
         idathr = jyr * 100000 + jjul * 100 + jhr


c if jdate not reached, read new record:
         if(idathr.LT.jdate) go to 17 
         if(idathr.EQ.jdate)then

c ---       Check station ID
            if(id.NE.ifstn(k)) then
               write(*,*)'FATAL Error in RDWRITS -- Station ID bad'
               write(io6,*)'FATAL Error in RDWRITS -- Station ID bad'
               write(io6,*)' File number (CD144): ',k
               write(io6,*)' Expected Station ID: ',ifstn(k)
               write(io6,*)'    Found Station ID: ',id
               stop
            endif

c     Substitute total cloud cover if opaque is missing
c            if(ccc.eq.' ') ccc=tcc
C     CONVERT WD (tens of degrees) TO WIND DIRECTION (degrees)
c     Check for missing data
            if (cwd.eq.'  ')then
               xwd(index) = 9999.
               xws(index) = 9999.
               ichkws = 1
            else
               read (cwd,390)jwd
               read (cws,390)jws
 390           format (i2)
               if(jws.GT.0)then
                  if(jwd.GT.0) then
                     xwd(index) = jwd * 10.
                  else
                     xwd(index) = 9999.
                     xws(index) = 9999.
                     ichkws = 1
                  endif
               else
                  xwd(index) = 0.
               endif
               if (xwd(index).lt.0.0.or.xwd(index).gt.360.0) then
                  xwd(index) = 9999.
               endif
            endif
C     CONVERT TEMPERATURE FROM DEG F TO DEG K
c     Check for missing data
            if (ctemp.eq.'   ')then
               xtempk(index) = 9999.
            else
               read (ctemp,391)jtemp
 391           format (i3)
               xtempk(index) = (jtemp - 32) * 5./9. + 273.15
            endif
C     GET DEW POINT TEMPERATURE 
c     Check for missing data
            if (cdewp.eq.'   ')then
               dewp = 9999.
            else
               if(cdewp(1:1).eq.'X'.or.cdewp(1:1).eq.'x') then
                 read(cdewp,fmt='(1x,f2.0)') dewp
                 dewp=-dewp
               elseif(cdewp(1:1).eq.'0') then
                 read(cdewp,fmt='(1x,f2.0)') dewp
               else  
                 read (cdewp,fmt='(f3.0)') dewp
               endif
            endif
C     CONVERT WS FROM KNOTS TO M/S
            if (ichkws.ne.1) xws(index) = jws * 0.51444
C     CONVERT CD144 PRECIPITATION DATA TO PRECIP CODES AND
C     DATSAV2 WEATHER CODES
c     Check for missing data
            if (cprecip.eq.'        ')then
               ixpcode(index)=9999
c              ipwout=999
               ipwout(1)=999
               ipwout(2)=999
               ipwout(3)=999
            else
               read(cprecip,394)(jprecip(kkk),kkk=1,5)
 394           format (1x,5i1)
               call pcodes(io6,idathr,id,jprecip,ixpcode(index))
c              ipwout=0
               ipwout(1)=0
               ipwout(2)=0
               ipwout(3)=0
               idatsvi=1
               do isami=1,8
                  if(cprecip(isami:isami).ne.'0'.and.
     &               cprecip(isami:isami).ne.' ') then
                     read(cprecip(isami:isami),fmt='(i1)') ipw144
                     ipw144=ipw144-1
                     ipwout(idatsvi)=sam2dats(ipw144,isami)
                     idatsvi=idatsvi+1
                     if(idatsvi.gt.3) exit
                  endif
               enddo
            endif
C     CONVERT CD144 VISIBILITY CODE TO MILES
            if(cvis.eq.'   ') then
               visb=99999.
            else
               read(cvis,fmt='(i3)') ivisb
               visb=real(ivisb/10)
               ivisb=mod(ivisb,10)
               if(ivisb.le.6) then
                  visb=visb+real(ivisb)/16.
               else
                  visb=visb+.375+real(ivisb-6)*.125
               endif
            endif                         
C     CONVERT CD144 ALPHANUMERIC CLOUD DATA TO NUMERIC DATA
            call clouds(io6,cceil,ccc,ixcc(index),ixceil(index))
C     GET SEA-LEVEL PRESSURE
c     Check for missing data
            if (cslp.eq.'    ') then
               slp = 9999.
            else
               read (cslp,fmt='(f4.1)') slp
               if(slp.lt.400.) slp=slp+1000.
            endif

C     CONVERT SURFACE PRESSURE FROM INCHES HG TO MB
C     33.864 MB = 1 INCH HG
c     Check for missing data
            if (cpres.eq.'    ') then
c             If lpcalc selected, try estimating pressure from 
c               sea-level value first empirically, and then by
c               equation               
              if(lpcalc.and.slp.lt.9998.) then
                if(abs(xintercep(index)).lt.999..and.
     *             abs(xintercep(index)).gt.1.e-10) then
                   xpres(index)=(xintercep(index)+xslope(index)*
     *                          xtempk(index))*slp
                 else 
                  call slp2stp(slp,elev(k),xtempk(index),9999.,
     &                         xpres(index))
                 endif
              else
                 xpres(index) = 9999.
              endif
            else
               read (cpres,392) jpres
 392           format (i4)
               xpres(index) = jpres * 0.01* inHgtomb
            endif
C     FILL RELATIVE HUMIDITY OUTPUT ARRAY
c     Check for missing data
            if (crh.eq.'   ')then
c              if dry-bulb and dew point available, calculate
               if(tempk(index).lt.9998..and.dewp.lt.9998.) then
                 ixrh(index)=max(1,min(nint(100.*((173.-real(jtemp)
     &                 *.1+dewp)/(173.+real(jtemp)*.9))**8),100))
               else
                 ixrh(index) = 9999
               endif
            else
               read (crh,391) jrh
               ixrh(index) = jrh
            endif
c ---       Get precipitation rate from extended CD144 record
            if(LX144) then
c ---          Identify missing data (-99)
               if(pmmhr.LT.-90.0) then
                  xprate(index) = 9999.
               else
                  xprate(index) = pmmhr
               endif
            else
               xprate(index) = 9999.
            endif
         else
c     this is used if the whole record is missing.
c     backspace to recover at next hour (bdf)
            call dedat(ndate,nyr,njul,nhr)
            write(io6,1007) nyr,njul,nhr,id
            backspace(iofor)
            xws(index) = 9999.
            xwd(index) = 9999.
            ixceil(index) = 9999
            ixcc(index) = 9999
            xtempk(index) = 9999.
            ixrh(index) = 9999
            xpres(index) = 9999.
            ixpcode(index) = 9999
            xprate(index) = 9999.
c           ipwout=999
            ipwout(1)=999
            ipwout(2)=999
            ipwout(3)=999
            visb=99999.
         endif
c -------
c SAMSON:
c -------
      elseif (jdat.eq.2) then
         read(iofor,422,end=95)jyr,jmo,jday,jhr,iio,jcover1,ccc,
     >        temp,dewpt,jrh,jpres,jdir,spd,visb,jceilb,
     >        isampw
 422     format(1x,i2,3(1x,i2),1x,i1,38x,a1,a1,f6.1,f6.1,i4,i5,
     >        i4,f6.1,f7.1,i7,1x,9i1,23x,5i1)
         id=ifstn(iofor-ioff+1)
c ---    Copy present weather codes 2-6 to jprecip for precip codes
         do kkk=1,5
           jprecip(kkk)=isampw(kkk+1)
         enddo
c ---    Make sure year is YYYY (Y2K)
         call YR4(io6,jyr,ierr)
         if(ierr.NE.0) stop 'Halted in RDWRITS - see list file'
c     *-*-* EMI Modification (7/18/97)
c     ****  Change Hour 24 to Hour 0, next day instead of shifting
c     ****  data down by one hour
c     ****  jhr=jhr-1
         call julday(io6,jyr,jmo,jday,jjul)
         if(jhr.EQ.24) then
            jhr = 23
            call indecr(io6,jyr,jjul,jhr,1,0,23)
            call grday(io6,jyr,jjul,jmo,jday)
         endif
c     *-*-* End of EMI Modification (7/18/97)
         idathr = jyr * 100000 + jjul * 100 + jhr
         if(idathr.LT.jdate) then
c read new record
            goto 17 
         elseif(idathr.EQ.jdate)then
            ichkws=0
            if (jcover1.eq.'1') ccc='-'
            if (jcover1.eq.'9') ccc=' '
            xpres(index)=jpres
            if (jdir.eq.999) then
               xwd(index)=9999.
               xws(index)=9999.
               ichkws=1
            else
               if(spd.gt.0.) then
                  xwd(index)=jdir
               else
                  xwd(index)=0.
               endif
            endif
c SAMSON wind speed is in m/s
            if (ichkws.ne.1) xws(index)=spd
            if (temp.eq.9999.) then
               xtempk(index)=temp
            else
               xtempk(index)=temp+273.15
            endif
            if (jrh.eq.999) then
c           if dry-bulb and dew point available, calculate rh
              if(tempk(index).lt.9998..and.dewpt.lt.9998.) then
                temp=temp*1.8+32.
                dewpt=dewpt*1.8+32.
                ixrh(index)=max(1,min(nint(((173.-temp*.1
     &                 +dewpt)/(173.+temp*.9))**8),100))
              else
                ixrh(index)=9999
              endif
            else
               ixrh(index)=jrh
            endif
            if (jceilb.eq.77777) then
               cceil(1)='-'
               cceil(2)='-'
               cceil(3)='-'
            elseif (jceilb.eq.88888) then
               cceil(1)='8'
               cceil(2)='8'
               cceil(3)='8'
            elseif (jceilb.eq.99999) then
               cceil(1)=' '
               cceil(2)=' '
               cceil(3)=' '
            else
               jceilb=jceilb*3.2808399/100
               write(jceilc,425) jceilb
 425           format(I7)
               if (jceilc(5:5).eq.' ') jceilc(5:5)='0'
               if (jceilc(6:6).eq.' ') jceilc(6:6)='0'
               if (jceilc(7:7).eq.' ') jceilc(7:7)='0'
               cceil(3)=jceilc(7:7)
               cceil(2)=jceilc(6:6)
               cceil(1)=jceilc(5:5)
            endif
            call clouds(io6,cceil,ccc,ixcc(index),ixceil(index))
            do ii=1,5
               jprecip(ii)=jprecip(ii)+1
               if (jprecip(ii).eq.10) jprecip(ii)=0
            enddo
            if (iio.eq.9.and.jprecip(1).eq.0.and.jprecip(2).eq.0
     1           .and.jprecip(3).eq.0.and.jprecip(4).eq.0.and.
     2           jprecip(5).eq.0) then
               ixpcode(index)=9999
            else
               call pcodes(io6,idathr,id,jprecip,ixpcode(index))
            endif
c     decode present weather into DATSAV2 codes
c           ipwout=0
            ipwout(1)=0
            ipwout(2)=0
            ipwout(3)=0
c           if(minval(isampw).eq.9) then
            iminw=999
            do iw=1,9
              iminw=min(iminw,isampw(iw))
            end do
            if(iminw.eq.9) then
               if(iio.eq.9) then
c                 ipwout=999
                  ipwout(1)=999
                  ipwout(2)=999
                  ipwout(3)=999
              endif
            else
               idatsvi=1
               do isami=1,9
                  if(isampw(isami).ne.9) then
                     ipwout(idatsvi)=sam2dats(isampw(isami),isami)
                     idatsvi=idatsvi+1
                     if(idatsvi.gt.3) exit
                  endif
               enddo
            endif
c      visibilty km==>mi
            if(visb.lt.800.) then
              visb=min(visb/1.6093,99.9)
            else
              visb=99999.
            endif
         elseif(idathr.gt.jdate)then
c     this is used if the whole record is missing.
c     backspace to recover at next hour (bdf)
            call dedat(ndate,nyr,njul,nhr)
            write(io6,1007) nyr,njul,nhr,id
            backspace(iofor)
            xws(index) = 9999.
            xwd(index) = 9999.
            ixceil(index) = 9999
            ixcc(index) = 9999
            xtempk(index) = 9999.
            ixrh(index) = 9999
            xpres(index) = 9999.
            ixpcode(index) = 9999
c           ipwout=999
            ipwout(1)=999
            ipwout(2)=999
            ipwout(3)=999
            visb=99999.
         endif

c ------
c HUSWO:
c ------
      elseif (jdat.eq.3) then
         read(iofor,423,end=95) id,jyr,jmo,jday,jhr,
     &        itskc,ioskc,
     &        temp,dewp,jrh,jpresr,jdir,spd,
     &        visb,jceilb,pwth,
     &        iprec,
     &        precfl,isnow
 423     format(I5,1X,I4,3I2,1X,4X,1X,4X,1X,I2,I2,1X,F5.1,
     &        1X,1X,f5.1,1X,I3,1X,I4,1X,1X,I3,1X,F4.1,1X,
     &        f6.1,1X,I5,1X,A8,
     &        1X,2X,3X,1X,2X,3X,1X,2X,3X,2X,I3,A1,2X,I3)

c ---    Set units conversion
         lhmetric=.true.
         if(ihuswo.NE.2) lhmetric=.false.

c ---    Test temperature to see if data are in expected units
c        Metric  - temperature is in degrees C (must be < 61.0)
         if(temp.GT.60.0 .AND. temp.LT.999.0) then
            if(lhmetric) then
               write(io6,*)'FATAL Error in RDWRITS: HUSWO data units'
               write(io6,*)'Expected METRIC data units'
               write(io6,*)'Found ENGLISH temperature units, T = ',temp
               stop 'HALTED in RDWRITS: unexpected HUSWO data units'
            endif
         endif
         if(dewp.GT.60.0 .AND. dewp.LT.999.0) then
            if(lhmetric) then
               write(io6,*)'FATAL Error in RDWRITS: HUSWO data units'
               write(io6,*)'Expected METRIC data units'
               write(io6,*)'Found ENGLISH dew pt units, T = ',dewp
               stop 'HALTED in RDWRITS: unexpected HUSWO data units'
            endif
         endif

c ---    Test pressure to see if data are English or metric units
c ---    and compare with expected units
c        English - pressure is in hundredths of inches (>2000)
c        Metric  - pressure is in bars (<1101)
         if(jpresr.lt.1101) then
            if(.not.lhmetric) then
               write(io6,*)'FATAL Error in RDWRITS: HUSWO data units'
               write(io6,*)'Expected ENGLISH data units'
               write(io6,*)'Found METRIC pressure units, P = ',jpresr
               stop 'HALTED in RDWRITS: unexpected HUSWO data units'
            endif
         elseif(jpresr.lt.9990) then
            if(lhmetric) then
               write(io6,*)'FATAL Error in RDWRITS: HUSWO data units'
               write(io6,*)'Expected METRIC data units'
               write(io6,*)'Found ENGLISH pressure units, P = ',jpresr
               stop 'HALTED in RDWRITS: unexpected HUSWO data units'
            endif
         endif

c         id=ifstn(iofor-ioff+1)
c ---    Make sure year is YYYY (Y2K)
         call YR4(io6,jyr,ierr)
         if(ierr.NE.0) stop 'Halted in RDWRITS - see list file'
         call julday(io6,jyr,jmo,jday,jjul)
         if(jhr.EQ.24) then
            jhr = 23
            call indecr(io6,jyr,jjul,jhr,1,0,23)
            call grday(io6,jyr,jjul,jmo,jday)
         endif
         idathr = jyr * 100000 + jjul * 100 + jhr
         if(idathr.LT.jdate) then
c read new record
            goto 17             
         elseif(idathr.EQ.jdate)then

c ---       Check station ID
            if(id.NE.ifstn(k)) then
               write(*,*)'FATAL Error in RDWRITS -- Station ID bad'
               write(io6,*)'FATAL Error in RDWRITS -- Station ID bad'
               write(io6,*)' File number (HUSWO): ',k
               write(io6,*)' Expected Station ID: ',ifstn(k)
               write(io6,*)'    Found Station ID: ',id
               stop
            endif

c     process data:
c     process wind speed and direction:
            if (jdir.eq.999) then
               xwd(index) = 9999.0
               xws(index) = 9999.0
            else
               if (spd.eq.99.9) then
                  xwd(index) = 9999.0
                  xws(index) = 9999.0
               else
                  xwd(index) = jdir
                  if(lhmetric) then
c     wind speed already in m/s
                    xws(index) = spd
                  else
c     convert wind speed from mph to m/s:
                    xws(index) = spd*0.44704
                  endif
               endif
            endif
c     process ceiling height:
            if (jceilb.eq.99999) then
c     missing:
               ixceil(index) = 9999
            elseif (jceilb.eq.88888) then
c     cirroform:
               ixceil(index) = 888
            elseif (jceilb.eq.77777) then
c     unlimited:
               ixceil(index) = 999
            else
               if(lhmetric) then
c     convert from meters to hundreds of feet
                 ixceil(index) = nint(float(jceilb)/30.48)
               else
c     convert from feet to hundreds of feet
                 ixceil(index) = nint(float(jceilb)/100.0)
               endif
            endif
c     process sky cover (use total if opaque is missing):
            if (ioskc.ne.99) then
               ixcc(index) = ioskc
            elseif (itskc.ne.99) then
               ixcc(index) = itskc
            else
               ixcc(index) = 9999
            endif
c     process temperature:
            if (temp.eq.999.9) then
               xtempk(index) = 9999.0
            else
c     convert temperature from Celsius to Kelvin:
               if(lhmetric) then
                 xtempk(index) = temp+273.15
               else
c     convert temperature from Fahrenheit to Kelvin:
                 xtempk(index) = (temp-32)*5./9.+273.15
               endif
            endif
c     process relative humidity
            if (jrh.eq.999) then
c              if dry-bulb and dew point available, calculate
               if(temp.lt.998..and.dewp.lt.998.) then
c           if metric, convert temp and dewp to F
                 if(lhmetric) then
                   temp=temp*9./5. + 32.
                   dewp=dewp*9./5. + 32.
                 endif
                 ixrh(index)=max(1,min(nint(((173.-temp*.1
     &                  +dewp)/(173.+temp*.9))**8),100))
               else
                 ixrh(index) = 9999
               endif
            else
               ixrh(index) = jrh
            endif
c     process pressure:
            if (jpresr.eq.9999) then
               xpres(index) = 9999.0
            else
               if(lhmetric) then
c     pressure already in mBar
                 xpres(index) = float(jpresr)
               else
c     convert from hundreths of inches of Mercury to mBar:
                 xpres(index) = jpresr * 0.01 *inHgtomb
               endif
            endif
c     process precipitation data:
            call pcodes2(io6,idathr,id,pwth,ixpcode(index),nc)
c     count multiple weather occurences
            if (nc.gt.1) npwth = npwth + 1
c     generate weather codes
c           ipwout=999
            ipwout(1)=999
            ipwout(2)=999
            ipwout(3)=999
            if(pwth.ne.'99999999') then
c              ipwout=0
               ipwout(1)=0
               ipwout(2)=0
               ipwout(3)=0
               do idatsvi=1,3
                  isami=idatsvi*2-1
                  read(pwth(isami:isami+1),fmt='(i2)') ipwtemp
                  if(ipwtemp.ge.10) ipwout(idatsvi)=
     &               hus2dats(ipwtemp)
               enddo
            endif
            if(visb.lt.800.) then
c     convert visibility to miles if metric
              if(lhmetric) visb=min(visb/1.6093,99.9)
            else
              visb=99999.
            endif
         elseif(idathr.gt.jdate) then
c     no data, flag as missing and backspace
            call dedat(ndate,nyr,njul,nhr)
            write(io6,1007) nyr,njul,nhr,id
 1007       format(' No data at:',i4,1x,i3,1x,i2,', Station: ',i10)
            backspace(iofor)
            xws(index) = 9999.0
            xwd(index) = 9999.0
            ixceil(index) = 9999
            ixcc(index) = 9999
            xtempk(index) = 9999.0
            ixrh(index) = 9999
            xpres(index) = 9999.0
            ixpcode(index) = 9999
c           ipwout=999
            ipwout(1)=999
            ipwout(2)=999
            ipwout(3)=999
            visb=99999.
         endif
      
c ----------------------
C ISHWO, TD3505, TD9956:      
c ----------------------

      elseif (jdat.ge.5.and.jdat.le.7) then
c        Check if pressure calculation was selected, and attempt empirical
c          calculations
         if(LPCALC.and.LFIRSTRD(index)) then
            lfirstrd(index)=.false.
            call scanishwo(iofor,jdat,xintercep(index),xslope(index),
     *                     altrat(index))
c ---       Report results to list file
            write(io6,*)'Pressure analysis for station ',ifstn(k)
            write(io6,*)'   P(Stn)/P(MSL) = ',xintercep(index),
     &                  ' + T(Stn) * ',xslope(index)
            write(io6,*)'   P(Stn)/P(Altimeter) = ',altrat(index)
            write(io6,*)
         endif
         call GETISH(iofor,id,jyr,jmo,jday,jhr,xwd(index),xws(index),
     *               ixceil(index),ixcc(index),xtempk(index),
     *               ixrh(index),xpres(index),altim,slp,xprate(index),
     *               ixpcode(index),dtow,ipwout,visb,elevx,licc,
     *               istz(k),leof,1,jdat)
         if(leof .AND. altim.GT.9998. .AND. slp.GT.9998.) goto 95
         call julday(io6,jyr,jmo,jday,jjul)
         if(jhr.EQ.24) then
            jhr = 23
            call indecr(io6,jyr,jjul,jhr,1,0,23)
            call grday(io6,jyr,jjul,jmo,jday)
         endif
         idathr = jyr * 100000 + jjul * 100 + jhr
         if(idathr.LT.jdate) then
c     read new record
            goto 17             
         elseif(idathr.EQ.jdate)then

c ---       Check station ID
            if(id.NE.ifstn(k)) then
               write(*,*)'FATAL Error in RDWRITS -- Station ID bad'
               write(io6,*)'FATAL Error in RDWRITS -- Station ID bad'
               if(jdat.EQ.5) then
                  write(io6,*)' File number (ISHWO): ',k
               elseif(jdat.EQ.6) then
                  write(io6,*)'File number (TD3505): ',k
               else
                  write(io6,*)'File number (TD9956): ',k
               endif
               write(io6,*)' Expected Station ID: ',ifstn(k)
               write(io6,*)'    Found Station ID: ',id
               stop
            endif

c           if station pressure is missing, try to calculate it based
c           on altimeter setting first, and then on sea-level pressure.
c           for both, try the empirical calculation first
            if(xpres(index).gt.9998.) then
              if(elevx.lt.-400..or.elevx.gt.9998.) elevx=
     *           elev(k)
              if(altim.lt.9998.) then
                 if(altrat(index).gt.0.) then
                    xpres(index)=altim*altrat(index)
                 else
                    call alp2stp(altim,elevx,xpres(index))
                 endif
              endif
            endif
            if(xpres(index).gt.9998..and.slp.lt.9998.) then
                if(abs(xintercep(index)).lt.999..and.
     *             abs(xintercep(index)).gt.1.e-10) then
                  xpres(index)=(xintercep(index)+xslope(index)*
     *                         xtempk(index))*slp
               else 
                  call slp2stp(slp,elevx,xtempk(index),9999.,
     *                      xpres(index))
               endif
            endif
         else
c     this is used if the whole record is missing.
c     backspace to recover at next hour (bdf)
            call dedat(ndate,nyr,njul,nhr)
            write(io6,1007) nyr,njul,nhr,id
            backspace(iofor)
            xws(index) = 9999.
            xwd(index) = 9999.
            ixceil(index) = 9999
            ixcc(index) = 9999
            xtempk(index) = 9999.
            ixrh(index) = 9999
            xpres(index) = 9999.
            ixpcode(index) = 9999
            xprate(index) = 9999.
c           ipwout=999
            ipwout(1)=999
            ipwout(2)=999
            ipwout(3)=999
            visb=99999.
         endif
c --------------
c Generic Format:
c --------------
      elseif (jdat.eq.8) then
            read(iofor,*,end=95) jmo,jday,jyr,jhr,xctemp,xcprecip,
     1                         xcpres,xcrh,xcwd,xcws,xccc,xcceil
            jhr=int(jhr/100)

          ichkws = 0

c ---    Make sure year is YYYY (Y2K)

         call YR4(io6,jyr,ierr)


         if(ierr.NE.0) stop 'Halted in RDWRITS - see list file'
C     CONVERT DATE READ FROM FORMATTED FILE AND COMPARE TO DATE WANTED
         call julday(io6,jyr,jmo,jday,jjul)
         idathr = jyr * 100000 + jjul * 100 + jhr


c if jdate not reached, read new record:
         if(idathr.LT.jdate) go to 17 
         if(idathr.EQ.jdate)then
c
C     WIND DIRECTION - should be in degrees
c     Check for missing data
            if (xcwd.eq.9999..or.xcws.eq.9999.)then
               xwd(index) = 9999.
               xws(index) = 9999.
               ichkws = 1
            else
               xwd(index) = xcwd
               xws(index) = xcws
               if (xwd(index).lt.0.0.or.xwd(index).gt.360.0) then
                  xwd(index) = 9999.
                  xws(index) = 9999.
               endif
            endif
C     CONVERT TEMPERATURE FROM DEG C TO DEG K
c     Check for missing data
            if (xctemp.eq.9999)then
               xtempk(index) = 9999.
            else
               xtempk(index) = xctemp + 273.15
            endif
c
C     CONVERT PRECIPITATION DATA TO PRECIP CODES 
c     Check for missing data
            if (xcprecip.eq.9999)then
               ixpcode(index)=9999
            else
               if(xcprecip.gt.0.and.xtempk(index).gt.273.15) then
               ixpcode(index)=1
               elseif(xcprecip.gt.0.and.xtempk(index).le.273.15) then
               ixpcode(index)=2
               elseif(xcprecip.le.0) then
               ixpcode(index)=0
               endif
            endif
c
C     CONVERT CD144 ALPHANUMERIC CLOUD DATA TO NUMERIC DATA
c            call clouds(io6,cceil,ccc,ixcc(index),ixceil(index))
             if(xcceil.eq.9999) then
             ixceil(index)=9999
             else
             ixceil(index)=int(xcceil)
             endif
             if(xccc.eq.9999) then
             ixcc(index)=9999
             else
             ixcc(index)=int(xccc)
             endif
c
C     SURFACE PRESSURE is provided in MB
c     Check for missing data
            if (xcpres.eq.9999) then
             xpres(index) = 9999.
            else
             xpres(index) = xcpres
            endif
c
C     FILL RELATIVE HUMIDITY OUTPUT ARRAY
c     Check for missing data
            if (xcrh.eq.9999.)then
                 ixrh(index) = 9999
            else
                 ixrh(index) = anint(xcrh)
            endif
         else
c     this is used if the whole record is missing.
c     backspace to recover at next hour (bdf)
            call dedat(ndate,nyr,njul,nhr)
            write(io6,1007) nyr,njul,nhr,id
            backspace(iofor)
            xws(index) = 9999.
            xwd(index) = 9999.
            ixceil(index) = 9999
            ixcc(index) = 9999
            xtempk(index) = 9999.
            ixrh(index) = 9999
            xpres(index) = 9999.
            ixpcode(index) = 9999
            xprate(index) = 9999.
c           ipwout=999
            ipwout(1)=999
            ipwout(2)=999
            ipwout(3)=999
            visb=99999.
         endif         
      endif

c     Output the VSRN.DAT line
      if(lvsrnout) then
        if(visb.lt.10.) then
          write(iovsrn,fmt="(i6,6x,i4,4i2,t52,f4.2,
     *      3(2x,i2))") id,jyr,jmo,jday,jhr,0,visb,ipwout
        elseif(visb.lt.100.) then
          write(iovsrn,fmt="(i6,6x,i4,4i2,t52,f4.1,
     *      3(2x,i2))") id,jyr,jmo,jday,jhr,0,visb,ipwout
        else
          write(iovsrn,fmt="(i6,6x,i4,4i2,t52,f4.0,
     *      3(2x,i2))") id,jyr,jmo,jday,jhr,0,visb,ipwout
        endif
      endif
      

c     END LOOP OVER FORMATTED FILES (STATIONS)
c     ----------------------------------------
  95   continue

C     DECODE CURRENT DATE
      call dedat(ndate,jyr,jjul,jhr)
      call dedat(kdate,iyr,ijul,ihr)
C     WRITE SURFACE DATA TO OUTPUT FILE FOR THIS HOUR

         call wrs(io6,ioform,ntstn,iopack,iosurf,ibuf,jyr,jjul,jhr,
     &            xws,xwd,ixceil,ixcc,xtempk,ixrh,xpres,ixpcode,
     &            ncbdf,lfprint)

c --- Write precipitation rate data to PRECIP.DAT file
         if(LX144 .AND. envpop.EQ.1) call wrp(io6,ntstn,ioprec,
     &                         jyr,jjul,jhr,xprate,ncp,lfprint)
c
c -------------------------------
c --- Process count of valid data
c -------------------------------
c
c --- Count number of records
      ntotal=ntotal+1
c
c --- Count number of valid data points for station
      do n=1,ntstn
         if(xws(n).lt.xmissm)nvalid(n,1)=nvalid(n,1)+1
         if(xwd(n).lt.xmissm)nvalid(n,2)=nvalid(n,2)+1
         if(ixceil(n).lt.imiss)nvalid(n,3)=nvalid(n,3)+1
         if(ixcc(n).lt.imiss)nvalid(n,4)=nvalid(n,4)+1
         if(xtempk(n).lt.xmissm)nvalid(n,5)=nvalid(n,5)+1
         if(irh(n).lt.imiss)nvalid(n,6)=nvalid(n,6)+1
         if(xpres(n).lt.xmissm)nvalid(n,7)=nvalid(n,7)+1
         if(ixpcode(n).lt.imiss)nvalid(n,8)=nvalid(n,8)+1
c
c ---    Count number of calm winds at each station
         if(xws(n).eq.0.0)ncalms(n)=ncalms(n)+1
      enddo
c
         if(ndate.LT.iedathr)then
C     INCREMENT CURRENT DATE/HR BY ONE HOUR
            call indecr(io6,jyr,jjul,jhr,1,0,23)
            ndate = jyr * 100000 + jjul * 100 + jhr
C     INCREMENT CURRENT DATE/HR FOR EXISTING SURFACE INPUT DATA BY ONE HOUR
            call indecr(io6,iyr,ijul,ihr,1,0,23)
            kdate = iyr * 100000 + ijul * 100 + ihr
         endif
         if(ndate.GT.iedathr) then
            write(io6,300) ndate, iedathr
 300  format(1x,'ERROR in Subr. RDWRITS: NDATE, ',i8,' is greater ',
     1       'than the requested end date (IEDATHR), ',i8)
            write(*,987)
            go to 99
         endif
c
c --- End main loop (hours)
 55   continue

 99   continue

      if (npwth.gt.0) then
         write(io6,309) npwth
      endif
 309  format (1x,i5,' hours with multiple present weather codes,'
     &     ,/,' priority list is inverse of precipitation code list'
     &     ,' for surf.dat file')

c --- Report counts of missings
      if(LX144 .AND. envpop.EQ.1) then
         write(*,312)
         write(*,311) ncbdf(1),ncbdf(2),ncbdf(3),ncbdf(4),ncbdf(5),
     &                ncbdf(6),ncbdf(7),ncp
         write(io6,312)
         write(io6,311) ncbdf(1),ncbdf(2),ncbdf(3),ncbdf(4),ncbdf(5),
     &                  ncbdf(6),ncbdf(7),ncp
      else
         write(*,310)
         write(*,311) ncbdf(1),ncbdf(2),ncbdf(3),ncbdf(4),ncbdf(5),
     &                ncbdf(6),ncbdf(7)
         write(io6,310)
         write(io6,311) ncbdf(1),ncbdf(2),ncbdf(3),ncbdf(4),ncbdf(5),
     &                  ncbdf(6),ncbdf(7)
      endif

 310  format(' Number of records with all stations missing for',
     &     '   WS    WD ICEIL   ICC TEMPK   IRH  PRES')
 311  format('                                                ',
     1     8(I5,1X))
 312  format(' Number of records with all stations missing for',
     &     '   WS    WD ICEIL   ICC TEMPK   IRH  PRES  PRATE')
c
c ----------------------------------
c --- Write summary stats by station
c ----------------------------------
      write(io6,901)ntotal
      if(ntotal.gt.0)then
         xpcnt=100./float(ntotal)
      else
         xpcnt=0.0
      endif
c-----------------------------------------------------------------------
901   format(//'Total number of data records processed (NTOTAL) = ',i6/)
      write(io6,902)
902   format(16x,'Number of valid records & calms',41x,
     1 'Percentage of valid data & calms'/
     2 3x,'Station',6x,'WS',6x,'WD',1x,'Ceiling',3x,'Cloud',4x,
     3 'Temp',6x,'RH',3x,'Press',2x,'Precip',3x,'Calms',
     4 6x,'WS',6x,'WD',1x,'Ceiling',3x,'Cloud',4x,
     5 'Temp',6x,'RH',3x,'Press',2x,'Precip',3x,'Calms'/28x,'Height',3x,
     6 'Cover',28x,'Code',26x,'Height',3x,'Cover',28x,'Code')
c
      do n=1,ntstn
c
c ---    Compute percentages for printing
         do jj=1,8
            xpercent(jj)=nvalid(n,jj)*xpcnt
         enddo
            xpcalms=ncalms(n)*xpcnt
c
         write(io6,905)idstn(n),(nvalid(n,ii),ii=1,8),ncalms(n),
     1      xpercent,xpcalms
905      format(1x,i9,9(i8),9(f8.1))
      enddo
c-----------------------------------------------------------------------
c
      return
      end
c----------------------------------------------------------------------
      subroutine clouds(io,aceil,acc,jcc,jceil)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 010630           CLOUDS
c ---            Benjamin de Foy
c                Simplified version, does not need leading zeroes.
c
c --- Convert CD144 alphanumeric cloud cover and ceiling height data
c --- to numeric data
c
c --- UPDATES:
c     V5.0(010630) from V4.0(991223) (D. Strimaitis)
c     - Add listfile unit to arguments (remove reference to parameter)
c
c --- INPUTS:
c                IO - integer    - Fortran unit no. of list file
c             ACEIL - char*1     - Array (3) of CD144 ceiling ht data
c                                  for this hour
c               ACC - char*1     - Opaque cloud cover for this hour
c --- OUTPUT:
c               JCC - integer    - Numeric cloud cover for this station
c                                  this hour (tenths)
c             JCEIL - integer    - Numeric ceiling height for this
c                                  station this hour (hundreds of feet)
c
c --- CLOUDS called by:  RDWRITS
c --- CLOUDS calls:      none
c----------------------------------------------------------------------
      integer jcc, jceil
      character*1 aceil(3),acc
c use aceil2 to phase out aceil(3)
      character*3 aceil2

c write array of ceiling height characters to a 3 character string
      write(aceil2,'(3A1)') aceil(1),aceil(2),aceil(3)

c --- decode cloud cover
      if (acc.eq.' ')then
         jcc = 9999
      else
c use opaque sky cover
         if (acc.eq.'-') then
            jcc = 10
         else
            read(acc,'(I1)',err=21) jcc
         endif
      endif
      goto 22
 21   continue
      write(io,988) acc
 988  format(/,1x,'Error in subr. CLOUDS2: --- Unallowable character ',
     1     'in opaque sky cover field:  ACC = ',a1)
      write(*,987)
      stop
 22   continue

c --- decode ceiling height
      if (aceil2.eq.'   ') then
c missing ceiling height
         jceil = 9999
      elseif (aceil2.eq.'---') then
c if dashes, assume unlimited
         jceil = 999
      elseif (aceil2.eq.'888') then
c cirroform, avoid dealing with it for now, flag as missing
         jceil = 888
      else
         read(aceil2,'(I3)',err=25) jceil
      endif

      return

 25   continue
      write(io,100) aceil2
 100  format(/,1x,'Error in subr. CLOUDS: --- Unallowable character ',
     1     'in ceiling height field:  ACEIL = ',a3)
      write(*,987)
 987  format(1x,'ERROR in SMERGE run - see SMERGE.LST file')
      stop
      
      end
c----------------------------------------------------------------------
      subroutine pcodes(io,idate,id,iprec,ipcode)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 020308           PCODES
c ---            E. Insley, Earth Tech, Inc.

c                941215->991223: rewritten by B. de Foy
c
c --- Convert CD144 precipitation type data to two-digit precipitation
c     codes.  Multiple Codes are counted and reported.
c     For these cases, ipcode(1) has highest priority, ipcode(5) lowest.
c
c --- UPDATES:
c
c     V5.0(020308) from V5.0(010630) (D. Strimaitis)
c     - Use larger integer format when writing station ID
c
c     V5.0(010630) from V4.0(991223) (D. Strimaitis)
c     - Add listfile unit to arguments (remove reference to parameter)
c
c --- INPUTS:
c                IO - integer    - Fortran unit no. of list file
c             IDATE - integer    - Date for this hour YYYYJJJHH
c                ID - integer    - Station id
c             IPREC - integer(5) - Present Weather Codes.
c                                  (CD144: columns 25-29)
c
c --- OUTPUT:
c            IPCODE - integer    - Two-digit precipitation code
c
c --- PCODES called by:  RDWRITS
c --- PCODES calls:  DEDAT
c----------------------------------------------------------------------
      integer idate,id,iprec(5),ipcode
c
      ipcode = 0
      nc = 0
      do i = 5,1,-1
         ip = iprec(i)
         if (ip.ne.0) then
            ipcode = (i-1)*9+ip
            nc = nc + 1
         endif
      enddo

c CHECK FOR MORE THAN 1 TYPE OF PRECIPITATION REPORTED FOR SAME HOUR
      if (nc.gt.1) then
         call dedat(idate,nyr,njul,nhr)
         write(io,1000) nc,iprec,nyr,njul,nhr,id
 1000    format(' Multiple (',i2,') weather codes (',5I1,') at:',
     &        i4,1x,i3,1x,i2,', Station: ',i10)
      endif

      end
c----------------------------------------------------------------------
      subroutine pcodes2(io,idate,id,pwth,ipcode,nc)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 020308          PCODES2
c ---            B. de Foy, Earth Tech, Inc.
c
c
c --- Convert HUSWO weather type to two-digit precipitation codes
c     Priority scheme for multiple codes follows CD-144 convention -
c     lowest ipcode has highest priority
c     Note: there may be some difference between HUSWO and CD-144
c           ipcodes because HUSWO enables combinations of weather codes
c           that are not possible in CD-144, eg: light snow + light
c           snow pellets
c
c --- UPDATES:
c
c     V5.0(020308) from V5.0(010630) (D. Strimaitis)
c     - Use larger integer format when writing station ID
c
c     V5.0(010630) from V4.0(991223) (D. Strimaitis)
c     - Add listfile unit to arguments (remove reference to parameter)
c
c --- INPUTS:
c             IDATE - integer    - Date for this hour YYYYJJJHH
c                ID - integer    - Station id
c              PWTH - char*8     - Weather code (column 15 of HUSWO)
c
c --- OUTPUT:
c            IPCODE - integer    - Two-digit precipitation code
c
c --- OTHER VARIABLES:
c            ITABLE? - integer(10) - ipcode for HUSWO weather types
c
c --- PCODES called by:  RDWRITS
c --- PCODES calls:  DEDAT
c----------------------------------------------------------------------
      integer idate,id,iprec(4),ipcode,ipcoden
      integer itable2(10),itable3(10),itable4(10),itable5(10)
      integer itable6(10),itable9(10)
      character*8 pwth
      data itable2/ 1, 2, 3, 4, 5, 6, 7, 8, 9,99/
      data itable3/99,99,99,13,14,15,16,17,18,99/
      data itable4/19,20,21,22,23,24,26,26,26,99/
      data itable5/28,29,30,99,99,99,34,35,36,99/
      data itable6/37,38,39,41,41,41,44,44,44,99/
      data itable9/37,38,39,99,99,99,99,99,99,99/
c
      read(pwth,100) iprec(1),iprec(2),iprec(3),iprec(4)
 100  format(4I2)

c number of valid codes in pwth:
      nc = 0
c initialise ipcode:
      ipcode = 99

      do nf = 1,4
         ipcoden = 99
         if (iprec(nf).lt.20) then
c
         elseif (iprec(nf).lt.30) then
            ipcoden = itable2(iprec(nf)-19)
            nc = nc + 1
         elseif (iprec(nf).lt.40) then
            ipcoden = itable3(iprec(nf)-29)
            nc = nc + 1
         elseif (iprec(nf).lt.50) then
            ipcoden = itable4(iprec(nf)-39)
            nc = nc + 1
         elseif (iprec(nf).lt.60) then
            ipcoden = itable5(iprec(nf)-49)
            nc = nc + 1
         elseif (iprec(nf).lt.70) then
            ipcoden = itable6(iprec(nf)-59)
            nc = nc + 1
         elseif (iprec(nf).ge.90.and.iprec(nf).le.92) then
            ipcoden = itable9(iprec(nf)-89)
            nc = nc + 1
         endif
c copy ipcoden to ipcode if it has priority over ipcode (in cases of
c multiple codes)
         if (ipcoden.ne.99.and.(ipcoden.lt.ipcode.or.
     &        ipcode.eq.99)) then
            ipcode = ipcoden
         endif
      enddo

c Correct missing flag:
      if (ipcode.eq.99) then
         if (pwth.eq.'99999999') then
            ipcode = 9999
         else
            ipcode = 0
         endif
      endif

c check for multiple weather codes:
c (NB: weather codes < 20 and =>70 do not count for this)
      if (nc.gt.1) then
         call dedat(idate,nyr,njul,nhr)
         write(io,1000) nc,pwth,nyr,njul,nhr,id
 1000    format(' Multiple (',i2,') weather codes (',a8,
     &        ') at:',i4,1x,i3,1x,i2,', Station: ',i10)
      endif

      return
      end
c----------------------------------------------------------------------
      subroutine rdhd(iolst,iform,io,ibyr,ibjul,ibhr,
     1                ieyr,iejul,iehr,ibtz,nsta,ipack,datavers,
     2                maxs,idsta,anem,cname,clat,clon)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 041123             RDHD
c ---            J. Scire, Earth Tech, Inc.
c
c --- Read the header records from the SURF.DAT file
c
c --- UPDATES:
c --- Updated V5.6 (041123) from V5.3(030402) (F.Robe)
c     - Allow both old and new SURF.DAT formats. New format with
c       explicit time convention with date/hour and beginning/ending seconds
c     - read and pass data version number
c     - output to list file using explicit times
c
c     V5.3(030402) from V5.1(020809) (D. Strimaitis)
c     - New header record structure
c     - IENVPOP toggle removed
c     - Moved list-file output from COMP to here
c     V5.1(020809) from V5.0(020308) (D. Strimaitis)
c     - Add IENVPOP to toggle station info records
c     V5.0(020308) from V5.0(010630) (D. Strimaitis)
c     - Add new records for more station information
c     V5.0(010630) from V4.0(010315) (D. Strimaitis)
c     - Add listfile unit to arguments (remove reference to parameter)
c
c --- Updated 010315 from 961014 (D. Strimaitis)
c     - Y2K treatment of year enforced
c
c --- INPUTS:
c             IOLST - integer    - Fortran unit no. of list file
c             IFORM - integer    - Data format flag (0=data not used,
c                                  1=unformatted, 2=formatted)
c                IO - integer    - Fortran unit no. of input file
c              MAXS - integer    - Maximum number of stations
c --- OUTPUT:
c              IBYR - integer    - Beginning year of data (4 digits) - Hour-ending time convention
c             IBJUL - integer    - Beginning Julian day number- Hour-ending time convention
c              IBHR - integer    - Beginning hour- Hour-ending time convention
c              IEYR - integer    - Ending year of data (4 digits)- Hour-ending time convention
c             IEJUL - integer    - Ending Julian day number- Hour-ending time convention
c              IEHR - integer    - Ending hour- Hour-ending time convention
c              IBTZ - integer    - Base time zone (8 = PST, 7 = MST,
c                                  6 = CST, 5 = EST)
c              NSTA - integer    - Number of stations
c             IPACK - integer    - Data packing code (0=unpacked,
c                                  1=packed)
c          DATAVERS - charc*16   - Dataset version number 
c       IDSTA(MAXS) - int. array - Array of station identification
c                                  codes
c     ANEM(MAXS) - real array    - Anemometer height at each station (m)
c    CNAME(MAXS) - char*4 array  - Name of each station (----)
c     CLAT(MAXS) - char*16 array - Latitude of each station (deg[N/S])
c     CLON(MAXS) - char*16 array - Longitude of each station (deg[E/W])
c
c --- RDHD called by:  RDWRITS
c --- RDHD calls:      DEDAT, YR4, GRDAY 
c----------------------------------------------------------------------
c
      real anem(maxs)
      integer idsta(maxs)
      character*4 cname(maxs)
      character*16 clat(maxs),clon(maxs)

      character*4 xyunit
      character*8 datum, pmap ,axtz
      character*12 daten
      character*16 dataset,datavers
      character*64 datamod
      character*80 comment1

      integer ishift(3)

      write(iolst,81)
81    format(//,26x,'********************',//)
      write(iolst,*)'Data Read from Existing Surface Data Input File:'
      write(iolst,*)
      write(iolst,*)

      if(iform.eq.0)then
c ---    data not used
         nsta=0
         return
c
      else if(iform.eq.1)then
c
c ---    data unformatted
         read(io) dataset,datavers,datamod
c ---    Check for valid dataset name
         if(dataset.NE.'SURF.DAT') then
            write(iolst,*)
            write(iolst,*) 'RDHD:  Invalid previous SURF.DAT file'
            write(iolst,*) 'Dataset name found = ',dataset
            write(iolst,*) 'Dataset name expected = ','SURF.DAT'
            write(*,987)
            stop
         endif

         read(io) ncomment
         write(iolst,'(2a16,a64)') dataset,datavers,datamod
         write(iolst,'(i4)') ncomment
         do i=1,ncomment
            read(io) comment1
            write(iolst,'(a80)') comment1
         enddo
         read(io) pmap
         write(iolst,'(a8)') pmap
         if(pmap.EQ.'NONE   ') then
            if (datavers.eq.'2.1') then
c ---          read in extra line with UTC time zone (AXTZ)
               read(io)axtz
               write(iolst,'(a8)') axtz
               call utcbasr(axtz,xbtz)
               ibtz=int(xbtz)
c ---          explicit time with seconds  
               read(io) ibegn,ibsecn,iendn,iesecn,nsta,ipack
            else
c ---          hour-ending time convention (pre 2.1 dataset) 
               read(io) ibeg,iend,ibtz,nsta,ipack
            endif   
            read(io) (idsta(n),n=1,nsta)
         else
            read(io) datum,daten
            read(io) xyunit
            write(iolst,'(a8,a12)') datum,daten
            write(iolst,'(a4)') xyunit
            if (datavers.eq.'2.1') then
c ---          read in extra line with UTC time zone (AXTZ)
               read(io)axtz
               call utcbasr(axtz,xbtz)
               ibtz=int(xbtz)
c ---          explicit time with seconds  
               read(io) ibegn,ibsecn,iendn,iesecn,nsta,ipack
            else
c ---          hour-ending time convention (pre 2.1 dataset)041123
               read(io) ibeg,iend,ibtz,nsta,ipack
c ---          format time zone as character variable for output purpose
               xbtz=ibtz
               call basrutc(xbtz,axtz)
            endif   
            read(io) (idsta(n),n=1,nsta)
            read(io) (cname(n),n=1,nsta)
            read(io) (clat(n),n=1,nsta)
            read(io) (clon(n),n=1,nsta)
            read(io) (anem(n),n=1,nsta)
         endif
c

         if (datavers.eq.'2.1') then
c ---       decode starting and ending dates
            call dedat(ibegn,ibyrn,ibjuln,ibhrn)
            call dedat(iendn,ieyrn,iejuln,iehrn)
         else
c ---       decode starting and ending dates
            call dedat(ibeg,ibyr,ibjul,ibhr)
            call dedat(iend,ieyr,iejul,iehr)
         endif   

c
      else if(iform.eq.2)then
         ipack=0
         read(io,'(2a16,a64)') dataset,datavers,datamod
         if(dataset.NE.'SURF.DAT') then
            write(iolst,*)
            write(iolst,*) 'RDHD:  Invalid previous SURF.DAT file'
            write(iolst,*) 'Dataset name found = ',dataset
            write(iolst,*) 'Dataset name expected = ','SURF.DAT'
            write(*,987)
            stop
         endif
         read(io,*) ncomment
         write(iolst,'(2a16,a64)') dataset,datavers,datamod
         write(iolst,'(i4)') ncomment
         do i=1,ncomment
            read(io,'(a80)') comment1
            write(iolst,'(a80)') comment1
         enddo
         read(io,'(a8)') pmap
         write(iolst,'(a8)') pmap
         if(pmap.EQ.'NONE   ') then
            if (datavers.eq.'2.1') then
c ---          read in extra line with UTC time zone (AXTZ)
               read(io,'(a8)')axtz
               write(iolst,'(a8)') axtz
               call utcbasr(axtz,xbtz)
               ibtz=int(xbtz)
c ---          Explicit time convention -
               read(io,*)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,
     :                   iesecn,nsta
            else
c ---          hour-ending dataset 
               read(io,*)ibyr,ibjul,ibhr,ieyr,iejul,iehr,ibtz,nsta
c ---          format time zone as character variable for output purpose
               xbtz=ibtz
               call basrutc(xbtz,axtz)
            endif
            read(io,*)(idsta(n),n=1,nsta)
         else
            read(io,'(a8,a12)') datum,daten
            read(io,'(a4)') xyunit
            write(iolst,'(a8,a12)') datum,daten
            write(iolst,'(a4)') xyunit
            if (datavers.eq.'2.1') then
c ---          explicit time with seconds
               read(io,*)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,
     :                   iesecn,ibtz,nsta
            else
c ---          hour-ending dataset 
               read(io,*)ibyr,ibjul,ibhr,ieyr,iejul,iehr,ibtz,nsta
c ---          format time zone as character variable for output purpose
               xbtz=ibtz
               call basrutc(xbtz,axtz)
            endif
            do n=1,nsta
               read(io,*)idsta(n),cname(n),clat(n),clon(n),anem(n)
            enddo
         endif
      else
         write(iolst,12)iform
12       format(//2x,'ERROR IN SUBR. RDHD -- invalid value of IFORM'/
     1   5x,'IFORM = ',i10)
         write(*,987)
         stop
      endif


c --- Convert to/from hour-ending times from/to explicit times (041123)
c --- Hour-ending times are used internally in the program 
c --- Explicit times are used for input/output purposes only
      if (datavers.eq.'2.1') then
c ---    explicit time with seconds
c ---    Make sure years are YYYY (Y2K)  
         call YR4(io6,ibyrn,ierrb)
         call YR4(io6,ieyrn,ierre)
         if(ierrb.NE.0 .OR. ierre.NE.0) stop 'Halted in RDHD - Y2K'
c ---    converts seconds to hours
         if(ibsecn.GE.3600) then
            nhrinc=ibsecn/3600
            ibsecn=ibsecn-nhrinc*3600
            call INCR(io6,ibyrn,ibjuln,ibhrn,nhrinc)
         endif
         if(iesecn.GE.3600) then
            nhrinc=iesecn/3600
            iesecn=iesecn-nhrinc*3600
            call INCR(io6,ieyrn,iejuln,iehrn,nhrinc)
         endif

c ---    convert to hour-ending time (beginnig time only- ending time
c ---    is the same in hour-ending and explicit time conventions)
         ibyr=ibyrn
         ibjul=ibjuln
         ibhr=ibhrn
         call incr(io6,ibyr,ibjul,ibhr,+1)
         ieyr=ieyrn
         iejul=iejuln
         iehr=iehrn
      else
c ---    hour-ending dataset => compute explicit times
c ---    Make sure years are YYYY (Y2K)  
         call YR4(io6,ibyr,ierrb)
         call YR4(io6,ieyr,ierre)
         if(ierrb.NE.0 .OR. ierre.NE.0) stop 'Halted in RDHD - Y2K'
         ibyrn=ibyr
         ibjuln=ibjul
         ibhrn=ibhr
         ibsecn=0
         call incr(io6,ibyrn,ibjuln,ibhrn,-1)
         ieyrn=ieyr
         iejuln=iejul
         iehrn=iehr
      endif


C  WRITE OUT REMAINING SURFACE DATA INPUT FILE HEADER INFORMATION
        write(iolst,90) axtz, iform, ipack
90      format(3x,'Time Zone:', a8,/,3x,'File Format',
     2         ' (1=unformatted,2=formatted):',i3,/,3x,'Packing Code:',
     3         i3)
        call GRDAY(iolst,ibyrn,ibjuln,ibmon,ibdayn)
        call GRDAY(iolst,ieyrn,iejuln,iemon,iedayn)
        write(iolst,100) axtz,ibmon,ibdayn,ibyrn,ibhrn,ibsecn,
     :                        iemon,iedayn,ieyrn,iehrn,iesecn
100     format(/,3x,'Period (in time zone',a8,'):   ',i2,'/',i2,'/',
     1         i4,2x,i2,':',i4,'  to  ',i2,'/',i2,'/',i4,2x,i2,':',i4/)

C       WRITE THE AVAILABLE STATION NUMBERS IN THE EXISTING SURFACE
C       DATA INPUT FILE
C       SET VARIABLES FOR WRITING OUT IN 4 COLUMNS
C        J4 IS NO. ROWS IN A "SHORT" COLUMN
C        J5 IS NO. ROWS IN A "LONG" COLUMN
C        J6 IS THE NUMBER OF "LONG" COLUMNS
        j4 = nsta/4
        j6 = mod(nsta,4)
        if(j6.EQ.0)then
           j5 = j4
        else
           j5 = j4 + 1
        endif
        ishift(1) = j5
        do i=2,3
           if(i.LE.j6)then
              ishift(i) = ishift(i-1) + j5
           else
              ishift(i) = ishift(i-1) + j4
           endif
        enddo
        ncol = min0(nsta,4)
        write(iolst,120) (' ',k=1,ncol)
120     format( /,3x,'Stations Available in Existing Surface Data ',
     1         'Input File:  '/ 3x,4(a1,'No.',7x,'ID',8x)/)
        do i=1,j4
           i2 = i + ishift(1)
           i3 = i + ishift(2)
           i4 = i + ishift(3)
           write(iolst,130) i,idsta(i),i2,idsta(i2),i3,idsta(i3),i4,
     1                    idsta(i4)
130        format(3x,4(i3,2x,i10,6x))
        enddo
        if(j6.GT.0)then
           n1 = j5
           n2 = n1+(j6-1)*j5
           write(iolst,130) (k,idsta(k),k=n1,n2,j5)
        endif
      write(iolst,81)

      return
987   format(1x,'ERROR in SMERGE run - see SMERGE.LST file')
      end
c----------------------------------------------------------------------
      subroutine wrhd(ienvpop,iolst,iform,io,ibyr,ibjul,ibhr,
     1                ieyr,iejul,iehr,axtz,nsta,ipack,idsta,anem,
     2                cname,clat,clon,cver,clev)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 041123             WRHD
c ---            J. Scire, Earth Tech, Inc.

c
c --- Write the header records for a meteorological data file
c
c --- UPDATES:
c     V5.6(041123) from V5.3(030402) (F.Robe)
c     - Write explicit beginning/ending times with seconds 
c      (not hour-ending convention)
c     - Move time zone to its own header line with explicit
c       UTC time zone (not internal IBTZ)
c     - New dataset version number (2.1)
c     V5.3(030402) from V5.1(020809) (D. Strimaitis)
c     - New header record structure
c     V5.1(020809) from V5.0(020308) (D. Strimaitis)
c     - Add IENVPOP to toggle station info records
c     V5.0(020308) from V5.0(010630) (D. Strimaitis)
c     - Add new records for more station information
c     V5.0(010630) from V4.0(010315) (D. Strimaitis)
c     - Add listfile unit to arguments (remove reference to parameter)
c
c --- Updated 010315 from 961014 (D. Strimaitis)
c     - YYYY format for year
c
c --- INPUTS:
c           IENVPOP - integer    - Environment toggle for headers
c             IOLST - integer    - Fortran unit no. of list file
c             IFORM - integer    - Data format flag (1=unformatted,
c                                  2=formatted)
c                IO - integer    - Fortran unit no. of data file
c              IBYR - integer    - Beginning year of data (4 digits) (hour-ending)
c             IBJUL - integer    - Beginning Julian day number(hour-ending)
c              IBHR - integer    - Beginning hour(hour-ending)
c              IEYR - integer    - Ending year of data (4 digits)(hour-ending)
c             IEJUL - integer    - Ending Julian day number(hour-ending)
c              IEHR - integer    - Ending hour(hour-ending)
c              AXTZ - char*8     - UTC time zone (UTC-0800 = PST, UTC-0700 = MST,
c                                  UTC-0600 = CST, UTC-0500 = EST)
c              NSTA - integer    - Number of stations
c             IPACK - integer    - Data packing code (0=unpacked,
c                                  1=packed)
c       IDSTA(NSTA) - int. array - Array of station identification
c                                  codes
c     ANEM(NSTA) - real array    - Anemometer height at each station (m)
c    CNAME(NSTA) - char*4 array  - Name of each station (----)
c     CLAT(NSTA) - char*16 array - Latitude of each station (deg[N/S])
c     CLON(NSTA) - char*16 array - Longitude of each station (deg[E/W])
c           CVER - character*12  - Version of processor
c           CLEV - character*12  - Level of processor
c
c --- OUTPUT:  none  (but time variables are passed back and used afterwards)
c
c --- WRHD called by:  RDWRITS
c --- WRHD calls:      SHIFTSEC
c----------------------------------------------------------------------
c --- Local Variables
      real anem(nsta)
      integer idsta(nsta)
      character*4 cname(nsta)
      character*16 clat(nsta),clon(nsta)
      character*12 cver,clev
      character*1 q
      character*8 axtz

      character*4 xyunit
      character*8 datum, pmap
      character*12 daten
      character*16 dataset,dataver
      character*64 datamod
      character*80 comment1

c --- Configure output variables
      data dataset/'SURF.DAT'/, dataver/'2.1'/
      data datamod/'Hour Start and End Times with Seconds'/
      data ncomment/1/
      data comment1/'Produced by SMERGE Version: '/

c --- Set single quote character
      data q/''''/

c --- Construct the version-level comment string
      j=29
      do i=1,12
         if(cver(i:i).NE.' ') then
            comment1(j:j)=cver(i:i)
            j=j+1
         endif
      enddo
      j=j+1
      comment1(j:j+7)=' Level: '
      j=j+8
      do i=1,12
         if(clev(i:i).NE.' ') then
            comment1(j:j)=clev(i:i)
            j=j+1
         endif
      enddo

c --- Set map projection information
      if(ienvpop.EQ.0) then
         pmap='NONE    '
      elseif(ienvpop.EQ.1) then
         pmap='LL      '
         datum='WGS-G   '
         daten='10-10-2002  '
         xyunit='DEG '
      endif

c --- Convert hour-ending beg./ending times to explicit beg/end times 
c --- with seconds(041123)
      ibyrn=ibyr
      ibjuln=ibjul
      ibhrn=ibhr
      ibsecn=0
      call INCR(io6,ibyrn,ibjuln,ibhrn,-1)

      ieyrn=ieyr
      iejuln=iejul
      iehrn=iehr
      iesecn= 0

c --- Write records
      if(iform.eq.1)then
c ---    Unformatted file
c ---    code beginning and ending date/hour
         ibegn=ibyrn*100000+ibjuln*100+ibhrn
         iendn=ieyrn*100000+iejuln*100+iehrn
         write(io) dataset,dataver,datamod
         write(io) ncomment
         write(io) comment1
         write(io) pmap
         write(io) axtz

         if(ienvpop.EQ.0) then
            write(io)ibegn,ibsecn,iendn,iesecn,nsta,ipack
            write(io)idsta
         elseif(ienvpop.EQ.1) then
            write(io) datum,daten
            write(io) xyunit
            write(io)ibegn,ibsecn,iendn,iesecn,nsta,ipack
            write(io)idsta
            write(io)cname
            write(io)clat
            write(io)clon
            write(io)anem
         endif

      else if(iform.eq.2)then
c ---    Formatted file
         write(io,'(2a16,a64)') dataset,dataver,datamod
         write(io,'(i4)') ncomment
         write(io,'(a80)') comment1
         write(io,'(a8)') pmap
         write(io,'(a8)') axtz

         if(ienvpop.EQ.0) then
            write(io,10)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,
     :                  iesecn,nsta
10          format(2(i6,2i4,i6),i5)
            do i=1,nsta
               write(io,20)idsta(i)
            enddo
20          format(i8)
         elseif(ienvpop.EQ.1) then
            write(io,'(a8,a12)') datum,daten
            write(io,'(a4)') xyunit
            write(io,10)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,
     :                  iesecn,nsta
            do i=1,nsta
               write(io,21)idsta(i),q,cname(i),q,q,clat(i),q,q,
     &                     clon(i),q,anem(i)
            enddo
21          format(i8,2x,a1,a4,a1,2x,2(a1,a16,a1,2x),f10.2)
         endif
      else
         write(iolst,12)iform
12       format(//2x,'ERROR IN SUBR. WRHD -- invalid value of IFORM'/
     1   5x,'IFORM = ',i10)
         write(*,987)
987      format(1x,'ERROR in SMERGE run - see SMERGE.LST file')
         stop
      endif
c
      return
      end
c----------------------------------------------------------------------
      subroutine wrhdp(ienvpop,io,ibyr,ibjul,ibhr,ieyr,iejul,iehr,
     1                 axtz,nsta,idsta,cname,clat,clon,
     2                 cver,clev)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 041123            WRHDP
c ---            Adapted from WRHD
c
c --- Write the header records for a FORMATTED precipitation data file
c
c --- UPDATES:
c     V5.6(041123) from V5.3(030402) (F.Robe)
c     - Use explicit time with seconds (not hour-ending convention) 
c     - New dataset version number (2.1)
c     V5.3(030402) from V5.1(020809) (D. Strimaitis)
c     - New header record structure
c     V5.1(020809) from V5.0(020308) (D. Strimaitis)
c     - Add IENVPOP to toggle station info records
c
c --- INPUTS:
c           IENVPOP - integer    - Environment toggle for headers
c                IO - integer    - Fortran unit no. of data file
c              IBYR - integer    - Beginning year of data (4 digits) (hour-ending)
c             IBJUL - integer    - Beginning Julian day number(hour-ending)
c              IBHR - integer    - Beginning hour(hour-ending)
c              IEYR - integer    - Ending year of data (4 digits)(hour-ending)
c             IEJUL - integer    - Ending Julian day number(hour-ending)
c              IEHR - integer    - Ending hour(hour-ending)
c              AXTZ - char*8     - UTC time zone (UTC-0800 = PST, UTC-0700 = MST,
c                                  UTC-0600 = CST, UTC-0500 = EST)
c             NSTA - integer    - Number of stations
c       IDSTA(NSTA) - int. array - Array of station identification
c                                  codes
c    CNAME(NSTA) - char*4 array  - Name of each station (----)
c     CLAT(NSTA) - char*16 array - Latitude of each station (deg[N/S])
c     CLON(NSTA) - char*16 array - Longitude of each station (deg[E/W])
c           CVER - character*12  - Version of processor
c           CLEV - character*12  - Level of processor
c
c --- OUTPUT:  none
c
c --- WRHDP called by:  RDWRITS
c --- WRHDP calls:      SHIFTSEC
c----------------------------------------------------------------------
c --- Local Variables
      integer idsta(nsta)
      character*4 cname(nsta)
      character*16 clat(nsta),clon(nsta)
      character*12 cver,clev
      character*1 q
      character*8 axtz

      character*4 xyunit
      character*8 datum, pmap
      character*12 daten
      character*16 dataset,dataver
      character*64 datamod
      character*80 comment1

c --- Configure output variables
      data dataset/'PRECIP.DAT'/, dataver/'2.1'/
      data datamod/'Hour Start and End Times with Seconds'/
      data ncomment/1/
      data comment1/'Produced by SMERGE Version '/
                                                          
c --- Set single quote character
      data q/''''/

c --- Construct the version-level comment string
      j=28
      do i=1,12
         if(cver(i:i).NE.' ') then
            comment1(j:j)=cver(i:i)
            j=j+1
         endif
      enddo
      j=j+1
      comment1(j:j)='('
      j=j+1
      do i=1,12
         if(clev(i:i).NE.' ') then
            comment1(j:j)=clev(i:i)
            j=j+1
         endif
      enddo
      comment1(j:j)=')'

c --- Set map projection information
      if(ienvpop.EQ.0) then
         pmap='NONE    '
      elseif(ienvpop.EQ.1) then
         pmap='LL      '
         datum='WGS-G   '
         daten='10-10-2002  '
         xyunit='DEG '
      endif

c --- Convert hour-ending times to explicit times with seconds (041123)
      ibyrn=ibyr
      ibjuln=ibjul
      ibhrn=ibhr
      ibsecn=0
      call INCR(io6,ibyrn,ibjuln,ibhrn,-1)

      ieyrn=ieyr
      iejuln=iejul
      iehrn=iehr
      iesecn=0

c --- Write records

c ---    Formatted file
         write(io,'(2a16,a64)') dataset,dataver,datamod
         write(io,'(i4)') ncomment
         write(io,'(a80)') comment1
         write(io,'(a8)') pmap
         write(io,'(a8)') axtz

         if(ienvpop.EQ.0) then
            write(io,10)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,
     :                  iesecn,nsta
10          format(2(i6,2i4,i6),i5)
            do i=1,nsta
               write(io,20)idsta(i)
            enddo
20          format(i8)
         elseif(ienvpop.EQ.1) then
            write(io,'(a8,a12)') datum,daten
            write(io,'(a4)') xyunit
            write(io,10)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,
     :                  iesecn,nsta
            do i=1,nsta
              write(io,21)idsta(i),q,cname(i),q,q,clat(i),q,q,clon(i),q
            enddo
21          format(i8,2x,a1,a4,a1,2x,2(a1,a16,a1,2x))
         endif

      return
      end
c----------------------------------------------------------------------
      subroutine rds(iolst,iforms,nssta,ispack,io,iskip,ibuf,
     1    iyr,ijul,ihr,ws,wd,iceil,icc,tempk,irh,pres,ipcode)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 010630              RDS
c ---            J. Scire, SRC
c
c --- Read a record of surface meteorological data
c --- (missing value indicator = 9999. (real) or 9999 (integer))
c --- If packing is used, data will be unpacked before returning
c
c --- UPDATES:
c     V5.0(010630) from V4.0(010315) (D. Strimaitis)
c     - Add listfile unit to arguments (remove reference to parameter)
c
c --- Updated 010315 from 961014 (D. Strimaitis)
c     - Y2K treatment of year enforced
c
c --- INPUTS:
c             IOLST - integer    - Fortran unit no. of list file
c            IFORMS - integer    - Data format flag (1=unformatted,
c                                  2=formatted)
c             NSSTA - integer    - Number of surface stations
c            ISPACK - integer    - Data packing code (0=unpacked,
c                                  1=packed)
c                IO - integer    - Fortran unit no. of surface data
c             ISKIP - integer    - Flag controlling unpacking of data
c                                  if ISKIP=0 data are unpacked, if
c                                  ISKIP=1, data are read but not
c                                  unpacked (used only if ISPACK=1)
c     IBUF(3,NSSTA) - int. array - Buffer to temporarily store packed
c                                  data (used only if ISPACK = 1)
c
c --- OUTPUT:
c               IYR - integer    - Year of surface data (4 digits)
c              IJUL - integer    - Julian day of data
c               IHR - integer    - Hour of data
c         WS(NSSTA) - real array - Wind speed (m/s)
c         WD(NSSTA) - real array - Wind direction (degrees)
c      ICEIL(NSSTA) - int. array - Ceiling height (hundreds of ft)
c        ICC(NSSTA) - int. array - Opaque sky cover (tenths)
c      TEMPK(NSSTA) - real array - Temperature (deg. K)
c        IRH(NSSTA) - int. array - Relative humidity (percent)
c       PRES(NSSTA) - real array - Surface station pressure (mb)
c     IPCODE(NSSTA) - int. array - Precipitation code
c
c --- RDS called by:  RDWRITS
c --- RDS calls:    UNPCKS
c                   DEDAT
c                   YR4
c----------------------------------------------------------------------
c
      real ws(nssta),wd(nssta),tempk(nssta),pres(nssta)
c
      integer iceil(nssta),icc(nssta),irh(nssta),ipcode(nssta)
      integer ibuf(3,nssta)

      if(iforms.eq.1)then
c
c ---    unformatted data
         if(ispack.eq.0)then
c ---       read unpacked, unformatted data
            read(io)idathr,ws,wd,iceil,icc,tempk,irh,pres,ipcode
         else
c ---       read packed, unformatted data --
c ---       date/hr word + 3 words/station
            read(io)idathr,ibuf
c ---       unpack data (unless in skip mode)
            if(iskip.eq.0)call unpcks(nssta,ibuf,ws,wd,iceil,icc,
     1      tempk,irh,pres,ipcode)
         endif
c ---    decode date and time
         call dedat(idathr,iyr,ijul,ihr)
c
      else if(iforms.eq.2)then
c
c ---    formatted data
c-emi **** Modified by EMI 11/18/94 ****
c dgs -- Removed EMI change for the more general SURF.DAT record 3/1/99
         read(io,*)iyr,ijul,ihr,(ws(n),wd(n),iceil(n),icc(n),tempk(n),
     1   irh(n),pres(n),ipcode(n),n=1,nssta)
c         read(io,*)iyr,ijul,ihr
c         read(io,*) (ws(n),wd(n),iceil(n),icc(n),tempk(n),
c     1               irh(n),pres(n),ipcode(n),n=1,nssta)
c-emi **** End of EMI Modification ****
c
      else
         write(iolst,12)iforms
12       format(//2x,'ERROR IN SUBR. RDS -- invalid value of IFORMS'/
     1   5x,'IFORMS = ',i10)
         write(*,987)
987      format(1x,'ERROR in SMERGE run - see SMERGE.LST file')
         stop
      endif

c --- Make sure year is YYYY (Y2K)
      call YR4(iolst,iyr,ierr)
      if(ierr.NE.0) stop 'Halted in RDS - see list file'

      return
      end

c----------------------------------------------------------------------
      subroutine rdsn(iolst,iforms,nssta,ispack,io,iskip,ibuf,
     1    iyr,ijul,ihr,ws,wd,iceil,icc,tempk,irh,pres,ipcode)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 051005             RDSN
c ---            F.Robe, SRC
c
c --- Read a record of surface meteorological data
c --- with explicit beg./ending times with seconds (SURF.DAT version 2.1)
c --- (missing value indicator = 9999. (real) or 9999 (integer))
c --- If packing is used, data will be unpacked before returning
c
c               
c --- NOTE:    At this point (041123),only hourly records are allowed
c               and the output date still follows the hour-ending
c               convention
c
c --- UPDATES:
c     V5.6 (041123) to V5.601(051005)(F.Robe)
c     - Fixed typo which induced (Ending Day n Hour24) to be interpreted 
c       as  (day n, hour 23) instead of (day n+1, hour 0) 
c 
c --- INPUTS:
c             IOLST - integer    - Fortran unit no. of list file
c            IFORMS - integer    - Data format flag (1=unformatted,
c                                  2=formatted)
c             NSSTA - integer    - Number of surface stations
c            ISPACK - integer    - Data packing code (0=unpacked,
c                                  1=packed)
c                IO - integer    - Fortran unit no. of surface data
c             ISKIP - integer    - Flag controlling unpacking of data
c                                  if ISKIP=0 data are unpacked, if
c                                  ISKIP=1, data are read but not
c                                  unpacked (used only if ISPACK=1)
c     IBUF(3,NSSTA) - int. array - Buffer to temporarily store packed
c                                  data (used only if ISPACK = 1)
c
c --- OUTPUT:
c              IYR - integer    - Year of surface data (4 digits)(hour-ending)
c             IJUL - integer    - Julian day of data(hour-ending)
c              IHR - integer    - Hour of data(hour-ending)
c         WS(NSSTA) - real array - Wind speed (m/s)
c         WD(NSSTA) - real array - Wind direction (degrees)
c      ICEIL(NSSTA) - int. array - Ceiling height (hundreds of ft)
c        ICC(NSSTA) - int. array - Opaque sky cover (tenths)
c      TEMPK(NSSTA) - real array - Temperature (deg. K)
c        IRH(NSSTA) - int. array - Relative humidity (percent)
c       PRES(NSSTA) - real array - Surface station pressure (mb)
c     IPCODE(NSSTA) - int. array - Precipitation code
c
c --- RDSN called by:  RDWRITS
c --- RDSN calls:    UNPCKS , SHIFTEND 
c                   DEDAT
c                   YR4
c----------------------------------------------------------------------
c
      real ws(nssta),wd(nssta),tempk(nssta),pres(nssta)
c
      integer iceil(nssta),icc(nssta),irh(nssta),ipcode(nssta)
      integer ibuf(3,nssta)

      if(iforms.eq.1)then
c
c ---    unformatted data
         if(ispack.eq.0)then
c ---       read unpacked, unformatted data
            read(io)ibdathrn,ibsecn,iedathrn,iesecn,
     :                 ws,wd,iceil,icc,tempk,irh,pres,ipcode
         else
c ---       read packed, unformatted data --
c ---       date/hr word + 3 words/station
            read(io)ibdathrn,ibsecn,iedathrn,iesecn,ibuf
c ---       unpack data (unless in skip mode)
            if(iskip.eq.0)call unpcks(nssta,ibuf,ws,wd,iceil,icc,
     1      tempk,irh,pres,ipcode)
         endif

c ---    decode date and time
c ---    At this point only accept hourly datasets (041123)
         call deltsec(ibdathrn,ibsecn,iedathrn,iesecn,ndelsec)
         if ((ndelsec).ne.3600) then
            write(iolst,*)'Previous SURF.DAT must have hourly records'
            write(iolst,*)'Beginning time:',ibyrn,ibjuln,ibhrn,ibsecn
            write(iolst,* )'Ending time:',ieyrn,iejuln,iehrn,iesecn
            write(iolst,*)'STOP in subroutine RDSN'
            stop 'Halted in RDSN - see list file'
         endif
c ---       convert to single hour-ending time
c ---       explicit ending time = single hour-ending time
            call dedat(iedathrn,iyr,ijul,ihr)
c
c
      else if(iforms.eq.2)then
c
c ---    formatted data
         read(io,*)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,
     1      iesecn,(ws(n),wd(n),iceil(n),icc(n),tempk(n),
     2      irh(n),pres(n),ipcode(n),n=1,nssta)

c ---       Make sure year is YYYY (Y2K)
            call YR4(iolst,ibyrn,ierr)
            if(ierr.NE.0) stop 'Halted in RDS - Y2K - see list file'

c ---       Convert seconds to hours and hours to 0-23
            if (ibhrn.ge.24) then
               ibsecn=ibsecn+(ibhrn-23)*3600
               ibhrn=23
            endif
            if(ibsecn.GE.3600) then
               nhrinc=ibsecn/3600
               ibsecn=ibsecn-nhrinc*3600
               call INCR(io6,ibyrn,ibjuln,ibhrn,nhrinc)
            endif
            if (iehrn.ge.24) then
               iesecn=iesecn+(iehrn-23)*3600
               iehrn=23
            endif
            if(iesecn.GE.3600) then
               nhrinc=iesecn/3600
               iesecn=iesecn-nhrinc*3600
               call INCR(io6,ieyrn,iejuln,iehrn,nhrinc)
            endif            

c ---       At this point only accept hourly datasets (041123)
            ibdathrn=ibyrn*100000+ibjuln*100+ibhrn
            iedathrn=ieyrn*100000+iejuln*100+iehrn

            call deltsec(ibdathrn,ibsecn,iedathrn,iesecn,ndelsec)
            if (ndelsec.ne.3600) then
              write(iolst,*)'Previous SURF.DAT must have hourly records'
              write(iolst,*)'Beginning time:',ibyrn,ibjuln,ibhrn,ibsecn
              write(iolst,* )'Ending  time:',ieyrn,iejuln,iehrn,iesecn
              write(iolst,*)'STOP in subroutine RDS'
              stop 'Halted in RDSN - see list file'
            endif

c ---       convert to single hour-ending time
c ---       explicit ending time = single hour-ending time
            iyr=ieyrn
            ijul=iejuln
            ihr=iehrn
      else
         write(iolst,12)iforms
12       format(//2x,'ERROR IN SUBR. RDS -- invalid value of IFORMS'/
     1   5x,'IFORMS = ',i10)
         write(*,987)
987      format(1x,'ERROR in SMERGE run - see SMERGE.LST file')
         stop
      endif


      return
      end

c----------------------------------------------------------------------
      subroutine unpcks(nssta,ibuf,ws,wd,iceil,icc,tempk,irh,pres,
     1 ipcode)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 941215           UNPCKS
c ---            J. Scire, SRC
c
c --- Unpack an array of surface meteorological data using integer
c --- packing (3 words/station)
c        Word 1:  TTTTPCRRR --  TTTT = temp. (XXX.X deg, K)
c                                 PC = precipitation code (XX)
c                                RRR = relative humidity (XXX. %)
c        Word 2: pPPPPCCWWW -- pPPPP = station pressure (pXXX.X mb,
c                                      with p = 0 or 1 only)
c                                 CC = opaque sky cover (XX tenths)
c                                WWW = wind direction (XXX. deg.)
c        Word 3:   HHHHSSSS --  HHHH = ceiling height (XXXX. hundreds
c                                      of feet)
c                               SSSS = wind speed (XX.XX m/s)
c
c --- INPUTS:
c             NSSTA - integer    - Number of surface stations
c     IBUF(3,NSSTA) - int. array - Array of packed data
c
c --- OUTPUT:
c         WS(NSSTA) - real array - Wind speed (m/s)
c         WD(NSSTA) - real array - Wind direction (degrees)
c      ICEIL(NSSTA) - int. array - Ceiling height (hundreds of ft)
c        ICC(NSSTA) - int. array - Opaque sky cover (tenths)
c      TEMPK(NSSTA) - real array - Temperature (deg. K)
c        IRH(NSSTA) - int. array - Relative humidity (percent)
c       PRES(NSSTA) - real array - Surface station pressure (mb)
c     IPCODE(NSSTA) - int. array - Precipitation code
c
c --- UNPCKS called by:  RDS
c --- UNPCKS calls:      none
c----------------------------------------------------------------------
c
      real ws(nssta),wd(nssta),tempk(nssta),pres(nssta)
c
      integer iceil(nssta),icc(nssta),irh(nssta),ipcode(nssta)
      integer ibuf(3,nssta)
c
      do 100 i=1,nssta
c
      iword1=ibuf(1,i)
      iword2=ibuf(2,i)
      iword3=ibuf(3,i)
c
c --- unpack temperature, precip. code, relative humidity
      it=iword1/100000
      ipc=iword1/1000-it*100
      ir=iword1-it*100000-ipc*1000
c
c --- use a standard missing value indicator of 9999 for all variables
      if(it.eq.9999)then
         tempk(i)=9999.
      else
         tempk(i)=float(it)/10.
      endif
c
      if(ipc.eq.99)then
         ipcode(i)=9999
      else
         ipcode(i)=ipc
      endif
c
      if(ir.eq.999)then
         irh(i)=9999
      else
         irh(i)=ir
      endif
c
c --- unpack station pressure, cloud cover, wind direction
      ip=iword2/100000
      ic=iword2/1000-ip*100
      iw=iword2-ip*100000-ic*1000
c --- NOTE: 1XXXX is allowed for station pressure and converts to
c ---       1XXX.X mb
      if(ip.eq.9999)then
         pres(i)=9999.
      else
         pres(i)=float(ip)/10.
      endif
c
      if(ic.eq.99)then
         icc(i)=9999
      else
         icc(i)=ic
      endif
c
      if(iw.eq.999)then
         wd(i)=9999.
      else
         wd(i)=iw
      endif
c
c --- unpack ceiling height, wind speed
      ih=iword3/10000
      is=iword3-ih*10000
c
      iceil(i)=ih
      if(is.eq.9999)then
         ws(i)=9999.
      else
         ws(i)=float(is)/100.
      endif
100   continue
c
      return
      end

c----------------------------------------------------------------------
      subroutine wrs(iolst,iforms,nssta,ispack,io,ibuf,iyr,ijul,ihr,
     1          ws,wd,iceil,icc,tempk,irh,pres,ipcode,ncbdf,lfprint)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 041123              WRS
c ---            J. Scire, SRC
c ---            Modified by E. Insley, SRC   3/30/94
c
c
c --- Write a record of surface meteorological data (one hour of data)
c --- (missing value indicator = 9999. (real) or 9999 (integer))
c --- Option to pack data if output data file is unformatted
c
c
c --- UPDATES:
c     V5.6 (041123) from V5.51(040922) (F.Robe)
c     - New data format (2.1) with explicit beginning/ending times
c       in date/hour and seconds
c     V5.51(040922) from V5.0(010630) (K. Morrison)
c     - Change dimension of ncbdf from 8 to 7 to be consistent with
c       RDWRITS
c     V5.0(010630) from V4.0(010315) (D. Strimaitis)
c     - Add listfile unit to arguments (remove reference to parameter)
c
c --- Updated 010315 from 991223 (D. Strimaitis)
c     - YYYY format for year (formats do not change!)
c
c --- Updated 991223 from 940330 (B. de Foy)
c     - Count hours where a variable is missing at all stations
c --- INPUTS:
c             IOLST - integer    - Fortran unit no. list file
c            IFORMS - integer    - Data format flag (1=unformatted,
c                                  2=formatted)
c             NSSTA - integer    - Number of surface stations
c            ISPACK - integer    - Data packing code (0=unpacked,
c                                  1=packed)
c                IO - integer    - Fortran unit no. of surface data
c     IBUF(3,NSSTA) - int. array - Buffer to temporarily store packed
c               IYR - integer    - Beginning Year of surface data (4 digits)(hour-ending)
c              IJUL - integer    - Beginning Julian day of data(hour-ending)
c               IHR - integer    - Beginning Hour of data(hour-ending)
c         WS(NSSTA) - real array - Wind speed (m/s)
c         WD(NSSTA) - real array - Wind direction (degrees)
c      ICEIL(NSSTA) - int. array - Ceiling height (hundreds of ft)
c        ICC(NSSTA) - int. array - Opaque sky cover (tenths)
c      TEMPK(NSSTA) - real array - Temperature (deg. K)
c        IRH(NSSTA) - int. array - Relative humidity (percent)
c       PRES(NSSTA) - real array - Surface station pressure (mb)
c     IPCODE(NSSTA) - int. array - Precipitation code
c          ncbdf(7) - int. array - count no. of missing or replaced hrs
c            lfprint - logical - .true. : print changes to screen
c
c --- OUTPUT:  none but IYR,IJUL,IHR are passed back to be used again
c
c --- WRS called by:  RDWRITS
c --- WRS calls:      PACKS , SHIFTSEC
c----------------------------------------------------------------------
c
      real ws(nssta),wd(nssta),tempk(nssta),pres(nssta)
c
      integer iceil(nssta),icc(nssta),irh(nssta),ipcode(nssta)
      integer ibuf(3,nssta)
      integer ncbdf(7)

      logical lfprint


c --- Convert single hour-ending time to explicit beg/end times with seconds
c --- for  output purposes only (041123)
      ibyrn=iyr
      ibjuln=ijul
      ibhrn=ihr
      ibsecn=0
      call INCR(io6,ibyrn,ibjuln,ibhrn,-1)

      ieyrn=iyr
      iejuln=ijul
      iehrn=ihr
      iesecn=0
c --- Write ending time as day n, hour 24 , rather than day n+1, hour 0
      if (iehrn.eq.0) then
         call INCR(io6,ieyrn,iejuln,iehrn,-1)
         iehrn=iehrn+1
      endif    

c if fill in of gaps disabled, count the number of gaps
c
c check for missing wind speed data - bdf
      do i = 1,nssta
         if (ws(i).ne.9999.0) goto 300
      enddo
      ncbdf(1) = ncbdf(1)+1
      if (lfprint.and.ncbdf(1).le.100) then
         write(iolst,40) 'wind speed',ibyrn,ibjuln,ibhrn,ibsecn
      endif
 40   format('Missing ',a15,' at: ',i4,1x,i3,1x,i2,1x,i4)
 300  continue
c check for missing wind direction data - bdf
      do i = 1,nssta
         if (wd(i).ne.9999.0) goto 301
      enddo
      ncbdf(2) = ncbdf(2)+1
      if (lfprint.and.ncbdf(2).le.100) then
         write(iolst,40) 'wind direction',ibyrn,ibjuln,ibhrn,ibsecn
      endif
 301  continue
c check for missing temperature data - bdf
      do i = 1,nssta
         if (tempk(i).ne.9999.0) goto 302
      enddo
      ncbdf(5) = ncbdf(5)+1
      if (lfprint.and.ncbdf(5).le.100) then
         write(iolst,40) 'temperature',ibyrn,ibjuln,ibhrn,ibsecn
      endif
 302  continue
c check for missing pressure data - bdf
      do i = 1,nssta
         if (pres(i).ne.9999.0) goto 303
      enddo
      ncbdf(7) = ncbdf(7)+1
      if (lfprint.and.ncbdf(7).le.100) then
         write(iolst,40) 'pressure',ibyrn,ibjuln,ibhrn,ibsecn
      endif
 303  continue
c check for missing sky ceiling data - bdf
      do i = 1,nssta
         if (iceil(i).ne.9999) goto 304
      enddo
      ncbdf(3) = ncbdf(3)+1
      if (lfprint.and.ncbdf(3).le.100) then
         write(iolst,40) 'sky ceiling',ibyrn,ibjuln,ibhrn,ibsecn
      endif
 304  continue
c check for missing sky cover data - bdf
      do i = 1,nssta
         if (icc(i).ne.9999) goto 305
      enddo
      ncbdf(4) = ncbdf(4)+1
      if (lfprint.and.ncbdf(4).le.100) then
         write(iolst,40) 'sky cover',ibyrn,ibjuln,ibhrn,ibsecn
      endif
 305  continue
c check for missing RH data - bdf
      do i = 1,nssta
         if (irh(i).ne.9999) goto 306
      enddo
      ncbdf(6) = ncbdf(6)+1
      if (lfprint.and.ncbdf(6).le.100) then
         write(iolst,40) 'RH',ibyrn,ibjuln,ibhrn,ibsecn
      endif
 306  continue



      if(iforms.eq.1)then
c ---    Unformatted option
c ---    code date and time into a single integer variable
         ibdathrn=ibyrn*100000+ibjuln*100+ibhrn
         iedathrn=ieyrn*100000+iejuln*100+iehrn
c
         if(ispack.eq.0)then
c
c ---       write unpacked data
            write(io)ibdathrn,ibsecn,iedathrn,iesecn,
     :               ws,wd,iceil,icc,tempk,irh,pres,ipcode
         else
c
c ---       pack and write data
            call packs(nssta,ws,wd,iceil,icc,tempk,irh,pres,ipcode,
     1      ibuf)
            write(io)ibdathrn,ibsecn,iedathrn,iesecn,ibuf
         endif
      else if(iforms.eq.2)then
c ---    Formatted option
c-emi **** Modified by EMI 3/30/94 ****
c-emi    write(io,*)iyr,ijul,ihr,(ws(n),wd(n),iceil(n),icc(n),
c-emi1   tempk(n),irh(n),pres(n),ipcode(n),n=1,nssta)

c ---    Explicit beginning/ending times with seconds
         write(io,10)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,iesecn
10       format(2(3i4,i5,3x))
         write(io,20) (ws(n),wd(n),iceil(n),icc(n),tempk(n),irh(n),
     1                 pres(n),ipcode(n),n=1,nssta)
 20      format(1x,f8.3,1x,f8.3,1x,i4,1x,i4,1x,f8.3,1x,i4,1x,
     &          f8.3,1x,i4)
c 20      format((2f12.6,2i5,f12.6,i5,f12.6,i5))
c-emi **** End of EMI Modification ****
      else
         write(iolst,30)iforms
30       format(//2x,'ERROR IN SUBR. WRS -- invalid value of IFORMS'/
     1   5x,'IFORMS = ',i10)
         write(*,987)
987      format(1x,'ERROR in SMERGE run - see SMERGE.LST file')
         stop
      endif
c
      return
      end
c----------------------------------------------------------------------
      subroutine wrp(iolst,nssta,io,iyr,ijul,ihr,prate,ncbdf,lfprint)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 041123              WRP
c ---            Adapted from WRS
c
c
c --- UPDATES:
c     V5.6 (041123) from (020308) (F.Robe)
c     - New data format (2.1) with explicit beginning/ending times
c
c --- INPUTS:
c             IOLST - integer    - Fortran unit no. list file
c             NSSTA - integer    - Number of surface stations
c                IO - integer    - Fortran unit no. of surface data
c               IYR - integer    - Beginning Year of surface data (4 digits) (hour ending)
c              IJUL - integer    - Beginning Julian day of data (hour ending)
c               IHR - integer    - Beginning Hour of data (hour ending)
c      PRATE(NSSTA) - real array - Precipitation rate (mm/hr)
c             ncbdf - integer    - count no. of missing or replaced hrs
c            lfprint - logical - .true. : print changes to screen
c
c --- OUTPUT:  none (but iyr,ijul,ihr are passed back and used again)
c
c --- WRP called by:  RDWRITS
c --- WRP calls:      SHIFTSEC
c----------------------------------------------------------------------
c
      real prate(nssta)
      integer ncbdf
      logical lfprint


c --- Convert single hour-ending time to explicit beg/end times with seconds
c --- (041123)
      ibyrn=iyr
      ibjuln=ijul
      ibhrn=ihr
      ibsecn=0
      call INCR(io6,ibyrn,ibjuln,ibhrn,-1)

      ieyrn=iyr
      iejuln=ijul
      iehrn=ihr
      iesecn=3600
      call INCR(io6,ieyrn,iejuln,iehrn,-1)

c --- Check for missing precip data
      do i = 1,nssta
         if (prate(i).ne.9999.0) goto 300
      enddo
      ncbdf = ncbdf+1
      if (lfprint.and.ncbdf.le.100) then
         write(iolst,40) 'Precip',ibyrn,ibjuln,ibhrn,ibsecn
      endif
40    format('Missing ',a15,' at: ',i4,1x,i3,1x,i2,1x,i4)
300   continue


c --- Write record
      write(io,10)ibyrn,ibjuln,ibhrn,ibsecn,ieyrn,iejuln,iehrn,iesecn,
     :            prate
10    format(2(3i4,i5,3x),1x,10f9.3,24(/,13x,10f9.3))

      return
      end
c----------------------------------------------------------------------
      subroutine packs(nssta,ws,wd,iceil,icc,tempk,irh,pres,ipcode,
     1 ibuf)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 941215            PACKS
c ---            J. Scire, SRC
c
c --- Pack a set of surface meteorological variables using integer
c --- packing (3 words/station)
c        Word 1:  TTTTPCRRR --  TTTT = temp. (XXX.X deg, K)
c                                 PC = precipitation code (XX)
c                                RRR = relative humidity (XXX. %)
c        Word 2: pPPPPCCWWW -- pPPPP = station pressure (pXXX.X mb,
c                                      with p = 0 or 1 only)
c                                 CC = opaque sky cover (XX tenths)
c                                WWW = wind direction (XXX. deg.)
c        Word 3:   HHHHSSSS --  HHHH = ceiling height (XXXX. hundreds
c                                      of feet)
c                               SSSS = wind speed (XX.XX m/s)
c
c --- INPUTS:
c             NSSTA - integer    - Number of surface stations
c         WS(NSSTA) - real array - Wind speed (m/s)
c         WD(NSSTA) - real array - Wind direction (degrees)
c      ICEIL(NSSTA) - int. array - Ceiling height (hundreds of ft)
c        ICC(NSSTA) - int. array - Opaque sky cover (tenths)
c      TEMPK(NSSTA) - real array - Temperature (deg. K)
c        IRH(NSSTA) - int. array - Relative humidity (percent)
c       PRES(NSSTA) - real array - Surface station pressure (mb)
c     IPCODE(NSSTA) - int. array - Precipitation code
c
c --- OUTPUT:
c     IBUF(3,NSSTA) - int. array - Array of packed data
c
c --- PACKS called by:  WRS
c --- PACKS calls:      none
c----------------------------------------------------------------------
      real ws(nssta),wd(nssta),tempk(nssta),pres(nssta)
c
      integer iceil(nssta),icc(nssta),irh(nssta),ipcode(nssta)
      integer ibuf(3,nssta)
c
      do 100 i=1,nssta
c
c --- pack temperature, precip. code, relative humidity into word 1
      if(tempk(i).eq.9999.)then
         it=9999
      else
         it=10.*tempk(i)+0.5
      endif
c
      if(ipcode(i).eq.9999)then
         ipc=99
      else
         ipc=ipcode(i)
      endif
c
      if(irh(i).eq.9999)then
         ir=999
      else
         ir=irh(i)
      endif
      ibuf(1,i)=it*100000+ipc*1000+ir
c
c --- pack station pressure, cloud cover, wind direction into
c --- word 2
      if(pres(i).eq.9999.)then
         ip=9999
      else
c ---    NOTE: 1XXX.X mb allowed for station pressure and converts to
c ---          1XXXX
         ip=10.*pres(i)+0.5
      endif
c
      if(icc(i).eq.9999)then
         ic=99
      else
         ic=icc(i)
      endif
c
      if(wd(i).eq.9999.)then
         iw=999
      else
         iw=wd(i)+0.5
      endif
      ibuf(2,i)=ip*100000+ic*1000+iw
c
c --- pack ceiling height, wind speed into word 3
      ih=iceil(i)
c
      if(ws(i).eq.9999.)then
         is=9999
      else
         is=100.*ws(i)+0.5
      endif
      ibuf(3,i)=ih*10000+is
100   continue
c
      return
      end
c----------------------------------------------------------------------
      subroutine chkhushd(io,hushd)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 010630         CHKHUSHD
c ---            Benjamin de Foy, Earth Tech, Inc.
c
c --- Checks that HUSWO file contains all data records.
c     Read statement in rdwrits does not allow for missing data fields.
c     Stops if check failed.
c
c --- UPDATES:
c     V5.0(010630) from V4.0(991223) (D. Strimaitis)
c     - Add listfile unit to arguments (remove reference to parameter)
c 
c --- INPUT:
c     hushd    - char*120 - Header of HUSWO file
c
c --- OTHER VARIABLES:
c     nmax     - integer  - length of header in characters
c     indexmax - integer  - maximum number of fields in HUSWO file
c
c     CHKHUSHD called by: COMP
c----------------------------------------------------------------------
c
      character*120 hushd
      integer j,n,nmax,index,indexref,indexmax
      parameter(nmax=120,indexmax=20)

      n = 1
      indexref = 1

 100  continue
c skip blanks
      if (hushd(n:n).eq.' ') then
         n = n + 1
         if (n.gt.nmax) goto 999
         goto 100
      endif
c find a number
      j = 1
 101  continue
      if (hushd(n+j:n+j).ne.' ') then
         j = j + 1
         if (n+j.gt.nmax) goto 999
         goto 101
      endif
c read the number
      read(hushd(n:n+j),*) index
      if (index.eq.indexref) then
         if (index.eq.indexmax) then
c we are done and may return
            return
         else
c look for next index
            indexref = indexref+1
            n = n+j+1
            goto 100
         endif
      endif

 999  continue

      write(io,*) 'HUSWO file should contain all fields'
      write(io,*) hushd
      write(*,*) 'HUSWO file should contain all fields'
      write(*,*) hushd

      stop
      end
c----------------------------------------------------------------------
      subroutine getish(ioin,id,iyr,imo,ida,ihr,wd,ws,iceil,icc,
     *  tempk,irh,pres,altim,slp,xprec,ipcode,dtow,ipwout,visb,
     *  elevx,licc,ixtz,leof,maxap,jdat)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 041123           GETISH
c ---            K. Morrison, Earth Tech
c
c --- PURPOSE:  Get a single hour of ISHWO data, passing back variables
c               that could be used for SURF.DAT, PRECIP.DAT, VSRN.DAT, and 
c               SEA.DAT files.  Since there may be multiple records for an 
c               individual hour, the program always carries out at least 
c               2 reads per hour, backspacing as necessary.
c               Note that station ID is the 6-digit USAF/WMO ID.
c
c --- UPDATES:
c --- Ver 5.6 Level 041123 from Ver. 5.5_draft lev. 040709(F.Robe)
c     - Replace IPWOUT/IPWOUT1 full array assignments by individual
c       array components
c
c --- Ver. 5.5_draft lev. 040709 from Ver. 5.44 lev. 040616 (K. Morrison)
c     - Add processing for TD3505 and TD9956 and obtain station
c       elevation
c     - Obtain and return present weather codes and visibility
c --- Ver. 5.44 lev. 040616 from Ver. 5.41 lev. 040205 (K. Morrison)
c     - Return altimeter and sea-level pressure to RDWRITS
c     - Drop passing of elevation since all pressure calculations now
c       in RDWRITS
c --- Ver. 5.41 lev. 040210 from Ver. 5.4 lev. 0040205 (K. Morrison)
c     - Add array for precip over last 6 hours for allocation of 
c       accumulated precipitation
c     - Correct identification and output of missing hours
c
c
c --- INPUTS:
c          IOIN  - integer - Input unit number for current file
c          LICC  - logical - Switch to use total sky cover if opaque
c                            sky cover is missing
c          IXTZ  - integer - Time zone for LST of current station
c         MAXAP  - integer - Maximum precip accumulation period
c          JDAT  - integer - File type (5=ISHWO, 6=TD3505, 7=TD9956)
c
c --- OUTPUTS:   
c          ID    - integer - Identifier of current station
c          IYR   - integer - Year of current observation
c          IMO   - integer - Month of current observation
c          IDA   - integer - Day of current observation
c          IHR   - integer - Hour of current observation
c          WS    - real    - Wind speed for current observation
c          WD    - real    - Wind direction for current observation
c          ICEIL - integer - Ceiling height for current observation
c          ICC   - integer - Cloud cover for current observation 
c          TEMPK - real    - Air temperature for current observation
c          IRH   - integer - Relative humidity for current observation
c          PRES  - real    - Station pressure for current observation 
c          ALTIM - real    - Altimeter setting for current observation
c          SLP   - real    - Sea-level pressure for current observation
c          XPREC - real    - Liquid precipitation for current observation 
c          IPCODE- integer - Precipitation code for current observation 
c          DTOW  - real    - Air-sea temperature difference for current 
c                            observation
c          IPWOUT- integer - Array of 3 present weather codes for current 
c                            observation
c          VISB  - real    - Visibility for current observation
c          ELEVX - real    - Station elevation (TD3505 & TD9956)
c          LEOF  - logical - Flag for EOF on current file
c
c --- LOCAL VARIABLES: 
c          IYRK - Most recent year
c         JDAYK - Most recent julian day
c          IHRK - Most recent hour
c          PREC - Precipitation over the previous 6 hours
c
c --- GETISH called by:  RDWRITS
c --- GETISH calls:      READISH, DELTT, INCR, GRDAY
c----------------------------------------------------------------------
c
c
      logical licc,leof
      integer iyrk(300),ijdayk(300),ihrk(300)
      integer*2 ipwout(3),ipwout1(3)
      real prec(6,300)/1800*0./
      data iyrk,ijdayk,ihrk/900*-1/
c      
c     first call
c
      k=ioin-6
      call readish(ioin,id,iyr,imo,ida,ihr,jday,wd,ws,iceil,
     *  icc,tempk,irh,pres,altim,slp,prec(6,k),ipdur,ipcode,dtow,
     *  ipwout,visb,elevx,licc,ixtz,leof,maxap,jdat)
      if(leof) goto 9999
c
c     check for missing periods
c
c     check if first read for this station
      if(iyrk(k).lt.0) then
        iyrk(k)=iyr
        ijdayk(k)=jday
        ihrk(k)=ihr      
c     check if gap in hours
      else
        call deltt(iyrk(k),ijdayk(k),ihrk(k),iyr,jday,ihr,jdiff)
        if(jdiff.gt.1) then
c       gap found - return missing values  
          backspace ioin
          call incr(6,iyrk(k),ijdayk(k),ihrk(k),1)
          iyr=iyrk(k)
          jday=ijdayk(k)
          ihr=ihrk(k)
          call grday(6,iyr,jday,imo,ida)
          ws=9999.
          wd=9999.
          iceil=9999
          icc=9999
          tempk=9999.
          irh=9999
          pres=9999.
          xprec=9999.
          ipcode=9999
          dtow=9999.
          prec(6,k)=9999.
c         ipwout=999
          ipwout(1)=999
          ipwout(2)=999
          ipwout(3)=999
          visb=99999.
          elevx=9999.
          goto 9999
        else
c         no gap
          iyrk(k)=iyr
          ijdayk(k)=jday
          ihrk(k)=ihr
          if(ipdur.gt.1) then
            ifhr=6-ipdur+1
            do ii=5,ifhr
              if(prec(ii,k).lt.9998.) prec(6,k)=prec(6,k)-prec(ii,k)
            enddo
          endif
          prec(6,k)=max(prec(6,k),0.)
          xprec=prec(6,k)      
        endif
      endif 
c
c     subsequent call(s)
c
100   call readish(ioin,id,iyr1,imo1,ida1,ihr1,jday1,wd1,ws1,iceil1,
     *  icc1,tempk1,irh1,pres1,altim1,slp1,xprec1,ipdur1,ipcode1,
     *  dtow1,ipwout1,visb1,elevx1,licc,ixtz,leof,maxap,jdat)
      iyrk(k)=iyr
      ijdayk(k)=jday
      ihrk(k)=ihr      
      if(leof) goto 9999
c
c     first, check if same hour
c     if not, backspace unit and return
c
      if(iyr1.ne.iyr.or.jday1.ne.jday.or.ihr1.ne.ihr) then
        backspace ioin
        goto 9999
      endif
c
c     check variable by variable, retaining the most recent or most valid
c

c     wind speed
      if(ws1.lt.9998.) ws=ws1
c     wind direction
      if(wd1.lt.9998.) wd=wd1
c     ceiling height
      if(iceil1.lt.9999) iceil=iceil1
c     cloud cover
      if(icc1.lt.9999) icc=icc1
c     temperature
      if(tempk1.lt.9998.) tempk=tempk1 
c     humidity      
      if(irh1.lt.9999) irh=irh1
c     pressure
      if(pres1.lt.9998.) pres=pres1
c     altimeter
      if(altim1.lt.9998.) altim=altim1
c     sea-level pressure      
      if(slp1.lt.9998.) slp=slp1
c     precipitation
      if(xprec1.lt.9998.) then
        if(ipdur1.gt.1) then
          ifhr=6-ipdur1+1
          do ii=5,ifhr
            if(prec(ii,k).lt.9998.) xprec1=xprec1
     *        -prec(ii,k)
          enddo
          xprec1=max(0.,xprec1)
        endif
        if(prec(6,k).lt.9999.) then
          if(xprec1.gt.prec(6,k)) then
            prec(6,k)=xprec1
            ipdur=ipdur1
          endif
        else
          prec(6,k)=xprec1
          ipdur=ipdur1
        endif
        xprec=prec(6,k)
      endif
c     precipitation code
      if(ipcode1.lt.9999) ipcode=ipcode1
c     air-sea Tdiff
      if(dtow1.lt.9998.) dtow=dtow1
c     weather codes
c     if(minval(ipwout1).lt.998)ipwout=ipwout1
      imin=999
      do i=1,3
         imin=min(imin,ipwout1(i))
      end do
      if (imin.lt.998) then
            ipwout(1)=ipwout1(1)
            ipwout(2)=ipwout1(2)
            ipwout(3)=ipwout1(3)
      endif   
c     visibility
      if(visb1.lt.99998.) visb=visb1
c     elevation
      if(elevx1.lt.9998.) elevx=elevx1
c
c     repeat the read
c
      goto 100
c
c     before returning, wind down the precipitation array
9999  do ii=1,5
        prec(ii,k)=prec(ii+1,k)
      enddo
      return
      end
c
c----------------------------------------------------------------------
      subroutine readish(ioin,id,iyr,imo,ida,ihr,jday,wd,ws,
     * iceil,icc,tempk,irh,pres,altim,slp,xprec,ipdur,ipcode,
     * dtow,ipwout,visb,elevx,licc,ixtz,leof,maxap,jdat)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 121203          READISH
c ---            K. Morrison
c
c --- PURPOSE:  Read a single record of full ISHWO, TD3505, or TD9956  
c     data and extract variables that are needed for SURF.DAT, 
c     PRECIP.DAT, VSRN.DAT, and SEA.DAT files.
c     The subroutine adjust the date and hour to LST, and returns as the 
c     station ID the 6-digit USAF/WMO ID.
c
c --- UPDATES:                                        
c
c --- Ver 5.7.0 lev 121203 from Ver 5.64 lev 080130
c     - Read quality flags as single character rather than integer (0-9)
c       now that non-integer entries may be present
c     - Missing ISH temperatures are 999.9, so reset check from 9999.8
c       to 999.8
c     - Recognize and skip additional codes in ISHWO file:
c       AU and AX
c       Precip section
c       AB, AD, AE, AH, AI, AK, AM, AN, AO, AP
c       Climate section
c       CB, CF, CG, CH, CI, CN, CO, CR, CT, CU, CV, CW, CX
c       Cloud & Solar section
c       GH, GK, GL, GM, GN, GO, GP, GQ, GR
c       Ground section
c       IB, IC
c       Temperature section
c       KB, KC, KD, KE, KF
c       Pressure section
c       MG, MH, MK
c       Wind section
c       OB, OE
c       Relative Humidity section
c       RHX
c       Soil Temperature section
c       ST
c
c --- Ver. 5.64 lev. 080130 from Ver. 5.6 lev. 041123 (D. Strimaitis)
c     - Revise time-window in READISH so that only reported times that
c       fall within the hour (HH-1)01 to HH00 are assigned to hour HH.
c       For example, observations at 1701, 1725, 1749, and 1800 are all
c       assigned to "hour 18".
c     - Fix logic in READISH that overstates the number of missing
c       hours.
c
c --- Ver. 5.6 lev. 041123 from Ver. 5.52 lev. 041026 (F.Robe)
c     - Replace IPWOUT full array assingments by individual components
c       assignments
c
c --- Ver. 5.52 lev. 041026 from Ver. 5.5_draft lev. 040709 (K. Morrison)
c     - Add TD3505 and TD9956 handling, and extract station elevation for
c
c --- Ver. 5.5_draft lev. 040709 from Ver. 5.44 lev. 040621 (K. Morrison)
c     - Add TD3505 and TD9956 handling, and extract station elevation for
c       these types
c     - Translate automated present weather codes to manual codes, return
c       these codes, and modify precipitation code generation
c
c --- Ver. 5.44 lev. 040621 from Ver. 5.43 lev. 040322 (K. Morrison)
c     - Return altimeter setting and sea-level pressure to GETISH
c     - Remove surface pressure estimation, moving it to RDWRITS
c     - Add call from SCANISHWO to allow empirical evaluations of
c       station pressure from altimeter setting or from sea-level
c       pressure and temperature
c
c --- Ver. 5.43 lev. 040322 from Ver. 5.42 lev. 040318 (K. Morrison)
c     - Corrections to pressure calculation from altimeter setting and
c       sea level pressure
c
c --- Ver. 5.42 lev. 040318 from Ver. 5.41 lev. 040210 (K. Morrison)
c     - Force direction to be missing for variable winds
c
c --- Ver. 5.41 lev. 040210 from Ver. 5.4 lev. 040204 (K. Morrison)
c     - Get TSKC from cover summation if direct value missing
c     - Correct present weather handling for code differences between
c       automatic and manual measurement
c     - Calculate station pressure from altimeter setting or sea level
c       pressure when station value is missing (requires station elevation)
c
c
c --- INPUTS:
c          IOIN  - integer - Input unit number for current file
c          LICC  - logical - Switch to use total sky cover if opaque
c                            sky cover is missing
c          IXTZ  - integer - Time zone for LST of current station
c         MAXAP  - integer - Maximum precip accumulation period
c          JDAT  - integer - File type (ISHWO=5, TD3505=6, TD9956=7)
c
c --- OUTPUTS:   
c          ID    - integer - Identifier of current station
c          IYR   - integer - Year of current observation
c          IMO   - integer - Month of current observation
c          IDA   - integer - Day of current observation
c          IHR   - integer - Hour of current observation
c          JDAY  - integer - Julian day
c          WS    - real    - Wind speed for current observation
c          WD    - real    - Wind direction for current observation
c          ICEIL - integer - Ceiling height for current observation
c          ICC   - integer - Cloud cover for current observation 
c          TEMPK - real    - Air temperature for current observation
c          IRH   - integer - Relative humidity for current observation
c          PRES  - real    - Station pressure for current observation 
c          ALTIM - real    - Altimeter setting for current observation
c          SLP   - real    - Sea-level pressure for current observation
c          XPREC - real    - Liquid precipitation for current observation 
c          LPDUR - integer - Duration of precipitation for current observation 
c          IPCODE- integer - Precipitation code for current observation 
c          DTOW  - real    - Air-sea temperature difference for current 
c                            observation
c          IPWOUT- integer - Array of 3 present weather codes for current 
c                            observation
c          VISB  - real    - Visibility for current observation
c          ELEVX - real    - Station elevation for TD3505 and TD9956
c          LEOF  - logical - Flag for EOF on current file
c          
c
c --- READISH called by:  GETISH, SCANISHWO 
c --- READISH calls:      ISHQC, JULDAY
c----------------------------------------------------------------------
c

c --- v5.7.0 (121203)
c --- Mandatory data section qa flags
      character*1 awdqc,awsqc,aceilqc,avisqc,atempqc,adpqc,aslpqc
c --- Additional data section qa flags
      character*1 alpqc,apwqc,askcqc,aaltqc,apresqc,atwqc

      character*2866 recin
      character*2 addtype
      character*1 addn,wdtype
      logical licc,leof
      integer ileap,mdays(12)/31,28,31,30,31,30,31,31,30,31,30,31/
      integer*2 ipwout(3)
      integer*2 isa2ism(100)/0,1,2,3,4,4,999,999,999,999,
     1 10,76,13,999,999,999,999,999,18,999,
     2 28,21,20,21,22,24,29,38,36,38,
     3 45,41,43,45,47,49,999,999,999,999,
     4 63,63,65,63,63,73,75,66,67,999,
     5 53,51,53,55,56,57,57,58,59,999,
     6 63,61,63,65,66,67,67,68,69,999,
     7 73,71,73,75,79,79,79,999,999,999,
     8 81,80,81,81,82,85,86,86,999,999,
     9 95,17,95,96,17,97,99,999,999,19/
      leof=.false.
      npw=0
c
c     set all variables to missing
c
      wd=9999.
      ws=9999.
      iceil=9999
      icc=9999
      tempk=9999.
      irh=9999
      pres=9999.
      altim=9999.
      slp=9999.
      ipcode=0
      xprec=0.
      dtow=9999.
      ipdur=1
      visb=99999.
c     ipwout=999
      ipwout(1)=999
      ipwout(2)=999
      ipwout(3)=999
      elevx=9999.
c
c     read the record
c
      read(ioin,fmt='(a2866)',end=9999) recin
c
c     read mandatory section
c
      if(jdat.eq.5) then
c
c     ISHWO section
c
c --- v5.7.0 (121203)
      read(recin,1001) nchar,idwmo,iwban,iyr,imo,ida,ihr,imin,
     *  wd,awdqc,wdtype,ws,awsqc,iceil,aceilqc,vis,avisqc,
     *  tempk,atempqc,dp,adpqc,slp,aslpqc
1001  format(i4,i6,i5,i4,i2,i2,i2,i2,6x,f3.0,a1,a1,
     *  f4.1,a1,i5,a1,2x,f6.3,a1,2x,f5.1,a1,f5.1,a1,f5.1,a1)
      nchar=nchar+77
      ipstart=82
c
      elseif(jdat.eq.6) then
c
c     3505 section
c
c --- v5.7.0 (121203)
      read(recin,1002) nchar,idwmo,iwban,iyr,imo,ida,ihr,imin,slat,
     *  slong,elevx,wd,awdqc,wdtype,ws,awsqc,iceil,aceilqc,vis,
     *  avisqc,tempk,atempqc,dp,adpqc,slp,aslpqc
1002  format(i4,i6,i5,i4,i2,i2,i2,i2,1x,f6.3,f7.3,5x,f5.0,
     *  9x,f3.0,a1,a1,f4.1,a1,i5,a1,2x,f6.3,a1,2x,f5.1,a1,f5.1,a1,
     *  f5.1,a1)
      nchar=nchar+104
      ipstart=109
c
      elseif(jdat.eq.7) then
c
c     9956 section
c
c --- v5.7.0 (121203)
      read(recin,1003) nchar,idwmo,iyr,imo,ida,ihr,imin,slat,
     *  slong,elevx,wd,awdqc,wdtype,ws,awsqc,iceil,aceilqc,vis,
     *  avisqc,tempk,atempqc,dp,adpqc,slp,aslpqc
1003  format(i4,i6,i4,i2,i2,i2,i2,f6.3,f7.3,5x,f5.0,
     *  9x,f3.0,a1,a1,f4.1,a1,i5,a1,2x,f6.3,a1,2x,f5.1,a1,f5.1,a1,
     *  f5.1,a1)
      nchar=nchar+98
      ipstart=103
c
      endif
c
c      
c     set station id to WMO
c
      id=idwmo
c
c     adjust hour and day from UTC to local
c
      ihr=ihr-ixtz
c --- Accept all minutes within the hour ENDING at HH00 as hour HH
c --- (e.g., 1701, 1726, and 1800 are assigned to hour 18;
c ---  1801 goes to hour 19)
c     if(imin.gt.15) ihr=ihr+1
      if(imin.gt.0) ihr=ihr+1
      if(ihr.gt.23) then
        ida=ida+1
        ihr=ihr-24
        ileap=0
        if(mod(iyr,4).eq.0.and.imo.eq.2) ileap=1
        if(ida.gt.mdays(imo)+ileap) then
          ida=1
          imo=imo+1
          if(imo.eq.13) then
            imo=1
            iyr=iyr+1
          endif
        endif
      else
        if(ihr.lt.0) then
          ihr=ihr+24
          ida=ida-1
          if(ida.lt.1) then
            imo=imo-1
            if(imo.lt.1) then
              iyr=iyr-1
              imo=12
              ida=31
            else
              ida=mdays(imo)
              if(imo.eq.2.and.mod(iyr,4).eq.0) ida=ida+1
            endif
          endif
        endif
      endif
c
c     calculate julian day
c
      call julday(2,iyr,imo,ida,jday)
c      
c     check winds
c
c --- Revise logic for wind observations
c --------------------
c      if(wd.gt.998..or.ishqc(iwdqc).eq.9) then
c        wd=9999.
c      else
c        if(wdtype.eq.'C') wd=0.
c        if(wdtype.eq.'V') wd=9999.
c      endif
c      if(ws.gt.999.8.or.ishqc(iwsqc).eq.9.or.wd.gt.9998.) then
c        ws=9999.
c      else
c        if(wdtype.eq.'C') ws=0.
c      endif
c      if(ws.lt.0.1) wd=0.
c --------------------

c -------------------------------------------------
c --- Hierarchy for interpreting wind observations
c ---    1.  Use type code C        --> calm
c ---    2.  Use type code V        --> missing
c ---    3.  Use valid WS < 0.1     --> calm
c ---    4.  Invalid WD             --> missing
c ---    5.  Invalid WS             --> missing
c ---    6.  Use type code '9'      --> missing
c -------------------------------------------------

      if(wdtype.EQ.'C' .OR. wdtype.EQ.'c') then
c ---    CALM (Accept wind observation type code C)
         ws=0.
         wd=0.
      elseif(wdtype.EQ.'V' .OR. wdtype.EQ.'v') then
c ---    Accept wind observation type code V
c ---    Set variable winds to missing
         ws=9999.
         wd=9999.
c --- v5.7.0 (121203)
      elseif(ws.LT.0.1 .AND. ishqc(awsqc).NE.9) then
c ---    Valid calm speed
         ws=0.
         wd=0.
c --- v5.7.0 (121203)
      elseif(wd.gt.998..or.ishqc(awdqc).eq.9) then
c ---    Invalid wind direction
         ws=9999.
         wd=9999.
c --- v5.7.0 (121203)
      elseif(ws.gt.998..or.ishqc(awsqc).eq.9) then
c ---    Invalid wind speed
         ws=9999.
         wd=9999.
      elseif(wdtype.EQ.'9') then
c ---    Accept MISSING type code
         ws=9999.
         wd=9999.
      endif
c --------------------

c
c     convert ceiling height from m to 100s of ft
c
c --- v5.7.0 (121203)
      if(iceil.gt.99998.or.ishqc(aceilqc).eq.9) then
        iceil=9999
      else
        if(iceil.eq.22000) then
          iceil=999
        else
          iceil=nint(float(iceil)/30.48)
        endif
      endif
c
c     convert visibility from km to miles
c
c --- v5.7.0 (121203)
      if(vis.gt.999.98.or.ishqc(avisqc).eq.9) then
        visb=99999.
      else
        visb=vis/1.6093
      endif
c
c     check temperature and calculate humidity if possible
c
c --- v5.7.0 (121203)
      if(tempk.gt.999.8.or.ishqc(atempqc).eq.9) then
        tempk=9999.
        irh=9999
      else
        tempk=tempk+273.15
c --- v5.7.0 (121203)
        if(dp.gt.999.8.or.ishqc(adpqc).eq.9) then
          irh=9999
        else
          tempf=(tempk-273.15)*1.8+32.
          tdewf=dp*1.8+32.
          irh=nint(100.*(((173.-0.1*tempf+tdewf)/(173.+0.9*tempf))**8))
          irh=max(min(irh,100),1)
        endif
      endif
c
c     check sea-level pressure
c
c --- v5.7.0 (121203)
      if(slp.gt.9999.8.or.ishqc(aslpqc).eq.9) slp=9999.
c
c     read additional variable section
c     first, check that additional variables are present
c
      read(recin(ipstart-3:ipstart-2),fmt='(a2)') addtype
      if(addtype.ne.'AD') return
c
100   if(ipstart.ge.nchar) return
      read(recin(ipstart:ipstart+1),fmt='(a2)') addtype
      read(recin(ipstart+2:ipstart+2),fmt='(a1)') addn
      ipstart=ipstart+3
c
c     check for remarks or general qc remarks and return
c
      if(addtype.eq.'RE'.or.addtype.eq.'EQ'.or.addtype.eq.'QN') 
     *  return
c
c     check for unused variables and skip the associated fields
c

c --- v5.7.0 (121203)
c --- Present weather obs automated occurence
      if(addtype.eq.'AU') then
        ipstart=ipstart+8
        goto 100
      endif
c --- Past weather obs summary of day
      if(addtype.eq.'AX') then
        ipstart=ipstart+6
        goto 100
      endif
c --- Liquid precip monthly total
      if(addtype.eq.'AB') then
        ipstart=ipstart+7
        goto 100
      endif
c --- Liquid precip greatest 24hr in month
      if(addtype.eq.'AD') then
        ipstart=ipstart+19
        goto 100
      endif
c --- Liquid precip number days with specific amt for month
      if(addtype.eq.'AE') then
        ipstart=ipstart+12
        goto 100
      endif
c --- Liquid precip max short duration for month
      if(addtype.eq.'AH'.OR. addtype.eq.'AI') then
        ipstart=ipstart+15
        goto 100
      endif
c --- Snow depth max on ground for month (cont.)
      if(addtype.eq.'AK') then
        ipstart=ipstart+12
        goto 100
      endif
c --- Snow greatest 24hr in month
      if(addtype.eq.'AM') then
        ipstart=ipstart+18
        goto 100
      endif
c --- Snow accumulation in month
      if(addtype.eq.'AN') then
        ipstart=ipstart+9
        goto 100
      endif
c --- Liquid precip by minute
      if(addtype.eq.'AO') then
        ipstart=ipstart+8
        goto 100
      endif
c --- Liquid precip 15-minute
      if(addtype.eq.'AP') then
        ipstart=ipstart+6
        goto 100
      endif
c --- CRN secondary liquid precip sub-hr
      if(addtype.eq.'CB') then
        ipstart=ipstart+10
        goto 100
      endif
c --- CRN hourly fan speed
      if(addtype.eq.'CF') then
        ipstart=ipstart+6
        goto 100
      endif
c --- CRN primary liquid precip sub-hr
      if(addtype.eq.'CG') then
        ipstart=ipstart+8
        goto 100
      endif
c --- CRN RH-T
      if(addtype.eq.'CH') then
        ipstart=ipstart+15
        goto 100
      endif
c --- CRN RH-T (cont.)
      if(addtype.eq.'CI') then
        ipstart=ipstart+28
        goto 100
      endif
c --- CRN battery voltage
      if(addtype.eq.'CN' .AND. addn.eq."1") then
        ipstart=ipstart+18
        goto 100
      endif
c --- CRN diagnostic
      if(addtype.eq.'CN' .AND. addn.eq."2") then
        ipstart=ipstart+18
        goto 100
      endif
c --- CRN secondary diagnostic
      if(addtype.eq.'CN' .AND. addn.eq."3") then
        ipstart=ipstart+16
        goto 100
      endif
c --- CRN secondary diagnostic (cont.)
      if(addtype.eq.'CN' .AND. addn.eq."4") then
        ipstart=ipstart+16
        goto 100
      endif
c --- COOP metadata
      if(addtype.eq.'CO' .AND. addn.eq."1") then
        ipstart=ipstart+5
        goto 100
      endif
c --- COOP time offset
      if(addtype.eq.'CO' .AND. addn.ne."1") then
        ipstart=ipstart+8
        goto 100
      endif
c --- CRN control
      if(addtype.eq.'CR') then
        ipstart=ipstart+7
        goto 100
      endif
c --- CRN sub-hourly T
      if(addtype.eq.'CT') then
        ipstart=ipstart+7
        goto 100
      endif
c --- CRN hourly T
      if(addtype.eq.'CU') then
        ipstart=ipstart+13
        goto 100
      endif
c --- CRN hourly T extreme
      if(addtype.eq.'CV') then
        ipstart=ipstart+27
        goto 100
      endif
c --- CRN sub-hourly wetness
      if(addtype.eq.'CW') then
        ipstart=ipstart+14
        goto 100
      endif
c --- CRN hourly GEONOR vibrating wire
      if(addtype.eq.'CX') then
        ipstart=ipstart+26
        goto 100
      endif
c --- Hourly solar radiation
      if(addtype.eq.'GH') then
        ipstart=ipstart+28
        goto 100
      endif
c --- Sunshine % of possible
      if(addtype.eq.'GK') then
        ipstart=ipstart+4
        goto 100
      endif
c --- Sunshine for month
      if(addtype.eq.'GL') then
        ipstart=ipstart+6
        goto 100
      endif
c --- Solar irradiance
      if(addtype.eq.'GM') then
        ipstart=ipstart+30
        goto 100
      endif
c --- Solar radiation
      if(addtype.eq.'GN') then
        ipstart=ipstart+28
        goto 100
      endif
c --- Net solar radiation
      if(addtype.eq.'GO') then
        ipstart=ipstart+19
        goto 100
      endif
c --- Modeled solar irradiance
      if(addtype.eq.'GP') then
        ipstart=ipstart+31
        goto 100
      endif
c --- Hourly solar angle
      if(addtype.eq.'GQ') then
        ipstart=ipstart+14
        goto 100
      endif
c --- Hourly extraterrestrial radiation
      if(addtype.eq.'GR') then
        ipstart=ipstart+14
        goto 100
      endif
c --- Hourly surface T
      if(addtype.eq.'IB' .AND. addn.eq."1") then
        ipstart=ipstart+27
        goto 100
      endif
c --- Hourly surface T
      if(addtype.eq.'IB' .AND. addn.eq."2") then
        ipstart=ipstart+13
        goto 100
      endif
c --- Pan evaporation
      if(addtype.eq.'IC') then
        ipstart=ipstart+25
        goto 100
      endif
c --- Average air T
      if(addtype.eq.'KB') then
        ipstart=ipstart+10
        goto 100
      endif
c --- Extreme air T for month
      if(addtype.eq.'KC') then
        ipstart=ipstart+14
        goto 100
      endif
c --- Heat-Cool degree days
      if(addtype.eq.'KD') then
        ipstart=ipstart+9
        goto 100
      endif
c --- Number days exceeding T in month
      if(addtype.eq.'KE') then
        ipstart=ipstart+12
        goto 100
      endif
c --- Hourly calculated T
      if(addtype.eq.'KF') then
        ipstart=ipstart+6
        goto 100
      endif
c --- Average P for day
      if(addtype.eq.'MG') then
        ipstart=ipstart+12
        goto 100
      endif
c --- Average P for month
      if(addtype.eq.'MH') then
        ipstart=ipstart+12
        goto 100
      endif
c --- Extreme P for month
      if(addtype.eq.'MK') then
        ipstart=ipstart+24
        goto 100
      endif
c --- Hourly wind at 1.5m (CRN stations)
      if(addtype.eq.'OB') then
        ipstart=ipstart+28
        goto 100
      endif
c --- Summary of day wind
      if(addtype.eq.'OE') then
        ipstart=ipstart+16
        goto 100
      endif
c --- Calculated RH
c --- RH=((6.11*10^(7.5*D/237.7+D))/(6.11*10^(7.5*T/237.3+T))*100
      if(addtype.eq.'RH' .AND. addn.eq."X") then
        ipstart=ipstart+3
        goto 100
      endif
c --- Soil T
      if(addtype.eq.'ST') then
        ipstart=ipstart+17
        goto 100
      endif


c     precipitation history
      if(addtype.eq.'AC') then
        ipstart=ipstart+3
        goto 100
      endif
c     precipitation bogus
      if(addtype.eq.'AG') then
        ipstart=ipstart+4
        goto 100
      endif
c     snow depth
      if(addtype.eq.'AJ') then
        ipstart=ipstart+14
        goto 100
      endif
c     snow accumulation
      if(addtype.eq.'AL') then
        ipstart=ipstart+7
        goto 100
      endif
c     past weather
      if(addtype.eq.'AY'.or.addtype.eq.'AZ') then
        ipstart=ipstart+5
        goto 100
      endif
c     runway visibility
      if(addtype.eq.'ED') then
        ipstart=ipstart+8
        goto 100
      endif
c     sky cover layer
      if(addtype.eq.'GA') then
        ipstart=ipstart+13
        goto 100
      endif
c     below-station cloud
      if(addtype.eq.'GG') then
        ipstart=ipstart+15
        goto 100
      endif
c     sunshine
      if(addtype.eq.'GJ') then
        ipstart=ipstart+5
        goto 100
      endif
c     hail size
      if(addtype.eq.'HL') then
        ipstart=ipstart+4
        goto 100
      endif
c     ground observation
      if(addtype.eq.'IA') then
        if(addn.eq.'1') then
          ipstart=ipstart+3
        else
          ipstart=ipstart+9
        endif
        goto 100
      endif
c     extreme air temperature
      if(addtype.eq.'KA') then
        ipstart=ipstart+10
        goto 100
      endif
c     pressure change
      if(addtype.eq.'MD') then
        ipstart=ipstart+11
        goto 100
      endif
c     geopotential height isobaric level
      if(addtype.eq.'ME') then
        ipstart=ipstart+6
        goto 100
      endif
c     present weather in vicinity
      if(addtype.eq.'MV') then
        ipstart=ipstart+3
        goto 100
      endif
c     supplemental wind
      if(addtype.eq.'OA') then
        ipstart=ipstart+8
        goto 100
      endif
c     gusts
      if(addtype.eq.'OC') then
        ipstart=ipstart+5
        goto 100
      endif
c     waves
      if(addtype.eq.'UA') then
        ipstart=ipstart+10
        goto 100
      endif
c     swells
      if(addtype.eq.'UG') then
        ipstart=ipstart+9
        goto 100
      endif
c     ice accretion
      if(addtype.eq.'WA') then
        ipstart=ipstart+6
        goto 100
      endif
c     water surface ice
      if(addtype.eq.'WD') then
        ipstart=ipstart+20
        goto 100
      endif
c     ice history
      if(addtype.eq.'WG') then
        ipstart=ipstart+11
        goto 100
      endif
c
c     useful variables
c
c
c     liquid precipitation, retain 1-hr value if available, otherwise
c     return accumulated value
      if(addtype.eq.'AA') then
c --- v5.7.0 (121203)
        read(recin(ipstart:ipstart+7),fmt='(i2,f4.1,i1,a1)') lpdur,prec,
     *    itrace,alpqc
        if(lpdur.eq.1) then
          ipdur=lpdur
c --- v5.7.0 (121203)
          if(prec.gt.999.8.or.ishqc(alpqc).eq.9) then
            xprec=9999.
          else
            xprec=prec
          endif
        else
          if(lpdur.le.maxap.and.xprec.gt.9998.) then
            ipdur=lpdur
c --- v5.7.0 (121203)
            if(prec.gt.999.8.or.ishqc(alpqc).eq.9) then
              xprec=9999.
            else
              xprec=prec
            endif
          endif
        endif
        if(xprec.lt.0.01) ipcode=0
        if(xprec.gt.0..and.xprec.lt.9998..and.tempk.lt.9998.) then
          if(tempk.ge.273.15) then 
            ipcode=1
          else
            ipcode=20
          endif
        endif
        ipstart=ipstart+8
        goto 100
      endif
c
c     automated present weather for ipcode and check precip,
c     converting automated pw code to manual pw code first
      if(addtype.eq.'AW') then
c --- v5.7.0 (121203)
        read(recin(ipstart:ipstart+2),fmt='(i2,a1)') ipw,apwqc
        if(ishqc(apwqc).ne.9.and.npw.lt.3) then
          ipw=isa2ism(ipw+1)
          npw=npw+1
          ipwout(npw)=ipw
          if(((ipw.le.19).or.(ipw.ge.30.and.ipw.le.35).or.
     *      (ipw.ge.40.and.ipw.le.49)).and.
     *      (xprec.lt.0.01.or.xprec.gt.9998.)) then
            ipcode=0
            xprec=0.
          else
c           predominantly liquid            
            if(ipw.eq.20.or.ipw.eq.21.or.(ipw.ge.23.and.ipw.le.25).or.
     *        (ipw.ge.50.and.ipw.le.69).or.(ipw.ge.80.and.ipw.le.84)
     *        .or.ipw.eq.91.or.ipw.eq.92) then
              ipcode=1
c           predominantly solid 
            elseif(ipw.eq.22.or.(ipw.ge.36.and.ipw.le.39).or.
     *        (ipw.ge.70.and.ipw.le.79).or.ipw.eq.85.or.ipw.eq.86.or.
     *        ipw.eq.96.or.ipw.eq.99) then
              if(ipcode.ne.1) ipcode=20
c           indeterminate, use temperature
            else
              if(tempk.lt.9998.) then
                if(tempk.ge.273.15) then
                  ipcode=1
                else
                  if(ipcode.ne.1) ipcode=20
                endif
              endif
            endif
          endif
        endif
        if(ipcode.eq.0.and.ipdur.gt.1) xprec=0.
        ipstart=ipstart+3
        goto 100
      endif
c
c     sky cover summation - overridden if sky condition present
      if(addtype.eq.'GD') then
c --- v5.7.0 (121203)
        read(recin(ipstart:ipstart+3),fmt='(i1,i2,a1)') iskcs1,
     *    iskcs2,askcsqc
        if(licc.and.icc.eq.9999) then
          if(iskcs2.eq.99) then
c --- v5.7.0 (121203)
            if(iskcs1.eq.9.or.ishqc(askcsqc).eq.9) then
              icc=9999
            else
              if(iskcs1.eq.0) icc=0
              if(iskcs1.eq.1) icc=2
              if(iskcs1.eq.2) icc=4
              if(iskcs1.eq.3) icc=8
              if(iskcs1.ge.4) icc=10
            endif
          else
            if(iskcs2.eq.0) icc=0
            if(iskcs2.eq.1) icc=1
            if(iskcs2.eq.2) icc=3
            if(iskcs2.eq.3) icc=4
            if(iskcs2.eq.4) icc=5
            if(iskcs2.eq.5) icc=6
            if(iskcs2.eq.6) icc=8
            if(iskcs2.eq.7) icc=9
            if(iskcs2.eq.8) icc=10
          endif
        endif
        ipstart=ipstart+12
        goto 100
      endif
c
c     sky condition for opaque or total cloud cover
      if(addtype.eq.'GF') then
c --- v5.7.0 (121203)
        read(recin(ipstart:ipstart+4),fmt='(2i2,a1)') itskc,ioskc,
     *    askcqc
        if(ioskc.eq.99) then
          if(licc) then
c --- v5.7.0 (121203)
            if(itskc.eq.99.or.ishqc(askcqc).eq.9) then
              itskc=99
            endif
            ioskc=itskc
          endif
        endif  
        if(ioskc.eq.0) icc=0
        if(ioskc.eq.1) icc=1
        if(ioskc.eq.2) icc=3
        if(ioskc.eq.3) icc=4
        if(ioskc.eq.4) icc=5
        if(ioskc.eq.5) icc=6
        if(ioskc.eq.6) icc=8
        if(ioskc.eq.7) icc=9
        if(ioskc.eq.8) icc=10
        if(ioskc.eq.99) icc=9999
        ipstart=ipstart+23
        goto 100
      endif
c
c     altimeter setting and station pressure  
      if(addtype.eq.'MA') then
c --- v5.7.0 (121203)
        read(recin(ipstart:ipstart+11),fmt='(2(f5.1,a1))') altim,
     *    aaltqc,pres,apresqc
        if(altim.gt.9999.8.or.ishqc(aaltqc).eq.9) altim=9999.
        if(pres.gt.9999.8.or.ishqc(apresqc).eq.9) pres=9999.
        ipstart=ipstart+12
        goto 100
      endif
c
c     manual present weather for ipcode and check precip
      if(addtype.eq.'MW') then
c --- v5.7.0 (121203)
        read(recin(ipstart:ipstart+2),fmt='(i2,a1)') ipw,apwqc
        if(ishqc(apwqc).ne.9.and.npw.lt.3) then
          npw=npw+1
          ipwout(npw)=ipw
          if(((ipw.le.19).or.(ipw.ge.30.and.ipw.le.35).or.
     *      (ipw.ge.40.and.ipw.le.49)).and.
     *      (xprec.lt.0.01.or.xprec.gt.9998.)) then
            ipcode=0
            xprec=0.
          else
c           predominantly liquid            
            if(ipw.eq.20.or.ipw.eq.21.or.(ipw.ge.23.and.ipw.le.25).or.
     *        (ipw.ge.50.and.ipw.le.69).or.(ipw.ge.80.and.ipw.le.84)
     *        .or.ipw.eq.91.or.ipw.eq.92) then
              ipcode=1
c           predominantly solid 
            elseif(ipw.eq.22.or.(ipw.ge.36.and.ipw.le.39).or.
     *        (ipw.ge.70.and.ipw.le.79).or.ipw.eq.85.or.ipw.eq.86.or.
     *        ipw.eq.96.or.ipw.eq.99) then
              if(ipcode.ne.1) ipcode=20
c           indeterminate, use temperature
            else
              if(tempk.lt.9998.) then
                if(tempk.ge.273.15) then
                  ipcode=1
                else
                  if(ipcode.ne.1) ipcode=20
                endif
              endif
            endif
          endif
        endif
        if(ipcode.eq.0.and.ipdur.gt.1) xprec=0.
        ipstart=ipstart+3
        goto 100
      endif
c
c     sea surface temperature
      if(addtype.eq.'SA') then
c --- v5.7.0 (121203)
        read(recin(ipstart:ipstart+4),fmt='(f4.1,a1)') tw,atwqc
        if(tempk.gt.9998..or.tw.gt.999.8.or.ishqc(atwqc).eq.9) then
          dtow=9999.
        else
          dtow=tempk-273.15-tw
        endif
        ipstart=ipstart+5
        goto 100
      endif
c
c     otherwise, there has been an error
        open(unit=600,file='isherror.txt')
        write(*,*) ' Error decoding ISH code'
        write(*,*) ' Check isherror.txt for message'
        write(600,*) ' Error in decoding at column ',ipstart
        write(600,*) ' Bad code : ',addtype,' in record :'
        write(600,*) recin(1:nchar)
        stop
c
c     end of data extraction
c
9999  leof=.true.
      return
      end
c----------------------------------------------------------------------
      integer function ishqc(aqc)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 121203            ISHQC
c ---            K. Morrison, Earth Tech
c
c --- PURPOSE:  Decode a QC flag for ISHWO data
c
c --- UPDATE:
c
c --- Ver 5.7.0 lev 121203 from Ver 5.41 lev. 040210
c     - Change the flag from integer to character to match update to
c       the data format specification
c     - Reject unknown character codes (as of Nov 7, 2012)
c     - Revise interpretation of quality flag = '9' to indicate that
c       the flag is missing, not the data.  The 'missing' assignment
c       conflicted with the "9: Passed gross limits check if element
c       is present" assignment.  This conflict is removed in the
c       Nov 7, 2012 ISH Format Document.
c       Note:  some additional data fields still say 9 is missing, and
c              these are interpreted to mean the flag is missing
c
c
c --- Ver. 5.41 lev. 040210 from Ver. 5.4 lev. 040204
c     - Accept suspect ISH values (codes 2 and 6)
c
c --- INPUTS:
c --- v5.7.0 (121203)
c          AQC   - character - QC flag
c
c --- OUTPUTS:   
c          ISHQC - integer - Decoded QC
c                            1 - Accept current value
c                            9 - Reject current value
c          
c
c --- ISHQC   called by:  READISH 
c --- ISHQC   calls:      none 
c----------------------------------------------------------------------
c --- v5.7.0 (121203)
      character*1 aqc
      logical lunknown
c --- Reject unknown character codes
      lunknown=.TRUE.
      if(ICHAR(aqc).GE.48 .AND. ICHAR(aqc).LE.57) then
c ---    (0-9) is OK
         lunknown=.FALSE.
      else
c ---    Test for known characters
         if(aqc.EQ.'A') lunknown=.FALSE.
         if(aqc.EQ.'C') lunknown=.FALSE.
         if(aqc.EQ.'I') lunknown=.FALSE.
         if(aqc.EQ.'M') lunknown=.FALSE.
         if(aqc.EQ.'P') lunknown=.FALSE.
         if(aqc.EQ.'R') lunknown=.FALSE.
         if(aqc.EQ.'U') lunknown=.FALSE.
      endif
      if(lunknown) then
         write(*,*)'ERROR in subroutine ISHQC'
         write(*,*)'Data quality flag is not known: ',aqc
         write(*,*)'Current flags are 0-9 and A,C,I,M,P,R,U'
         stop
      endif

c --- v5.7.0 (121203)
c --- Current character codes indicate values may be used, so test for
c --- restricted integer values
      if(aqc.EQ.'3' .OR. aqc.EQ.'7') then
        ishqc=9
      else
        ishqc=1
      endif
      return
      end

c----------------------------------------------------------------------
      subroutine rdssta
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 020308           RDSSTA
c ---            D. Strimaitis, Earth Tech, Inc.

c
c --- Read the station information for the input suface data files
c
c --- INPUTS:
c       Common block /CONTROL/
c          nff, 
c       Parameters: IO6, IOSSTA, MXFF
c --- OUTPUT:
c
c --- RDSSTA called by:  SETUP
c --- RDSSTA calls:      DEBLNK, ALLCAP
c----------------------------------------------------------------------
c
c --- Include parameters and commons
      include 'params.smg'
      include 'params.cal'
      include 'control.smg'
      include 'station.smg'

      integer num(mxff)

      character*70 line,blank70,break
      character*1 cstor1(mxcol),cstor2(mxcol)

      character*4 cname4
      character*16 clatNS, clonEW

      data num /mxff*0/

      data blank70(1:35)  /'                                   '/
      data blank70(36:70) /'                                   '/

c --- Read and check first line of file
      read(iossta,'(a70)') Line
      if(line(1:7).NE.'SURFACE') then
         write(io6,*)
         write(io6,*)
         write(io6,*) 'RDSSTA:  Invalid file content'
         write(io6,*) 'First line should begin with SURFACE = '
         write(io6,*) 'First line found: '
         write(io6,*) line
         write(io6,*)
         write(io6,*)
         stop
      endif

c --- Initilize output arrays
      do i=1,mxff
         fanem(i)=10.0
         cfname(i)='----'
         cflat(i)='----------------'
         cflon(i)='----------------'        
      enddo

c --- Set the section break text string
      read(iossta,'(a70)') break

c --- Process data from each section
10    line=blank70
      read(iossta,'(a70)',end=999) Line

      if(line.NE.break) then
c ---    Pass to character*1 array for processing
         do i=1,70
            cstor1(i)=line(i:i)
            cstor2(i)=' '
         enddo
c ---    Remove blank characters from string
         call DEBLNK(cstor1,1,70,cstor2,nlim)
c ---    Find the location of '=' and '!' (if present)
         neqs=0
         nend=nlim
         do i=nlim,1,-1
            if(cstor2(i).EQ.'=') neqs=i
            if(cstor2(i).EQ.'!') nend=i-1
         enddo
         nlim=nend
         if(neqs.EQ.0) then
            write(io6,*)
            write(io6,*)
            write(io6,*) 'RDSSTA:  Invalid file content'
            write(io6,*) 'Assignment line must include an = '
            write(io6,*) 'Assignment line found: '
            write(io6,*) line
            write(io6,*)
            write(io6,*)
            stop
         endif
c ---    Convert lower case letters to upper case
         call ALLCAP(cstor2,nlim)
c ---    Pass back to line
         line=blank70
         do i=1,nlim
            line(i:i)=cstor2(i)
         enddo
c ---    Assign to variable
         i1=neqs+1
         if(line(1:12).EQ.'STATION_NAME') then
            i2=i1+3
            cname4=line(i1:i2)
         elseif(line(1:10).EQ.'STATION_ID') then
            read(line(i1:nlim),'(i10)') istation
         elseif(line(1:13).EQ.'ANEMOMETER_HT') then
            if(i1.LT.nlim) read(line(i1:nlim),'(f10.2)') anemht
         elseif(line(1:8).EQ.'LATITUDE') then
            i2=MIN(16,nlim)
            clatNS=line(i1:nlim)
         elseif(line(1:9).EQ.'LONGITUDE') then
            i2=MIN(16,nlim)
            clonEW=line(i1:nlim)
         endif

      else      
c ---    End of section reached, so pass into storage arrays
c ---    Match to station ID
         do i=1,nff
            if(istation.EQ.ifstn(i)) then
               num(i)=num(i)+1
               if(anemht.GT.0.) fanem(i)=anemht
               cfname(i)=cname4
               cflat(i)=clatNS
               cflon(i)=clonEW
            endif
         enddo
c ---    Reset variables
         anemht=0.
         cname4='    '
         clatNS='                '
         clonEW='                '
      endif

c --- Get next line
      goto 10

999   continue
c --- Report results to list file
      write(io6,*)
      write(io6,*)'RDSSTA:  Information extracted from station file'
      write(io6,*)
      write(io6,*)'   ID     Name  Anem(m)     Lat               Lon'
      do i=1,nff
         write(io6,101)ifstn(i),cfname(i),fanem(i),cflat(i),
     &                 cflon(i),num(i)
      enddo
101   format(i8,2x,a4,f8.2,6x,2(a16,2x),i4)
      write(io6,*)
      write(io6,*)

      return
      end
c----------------------------------------------------------------------
      subroutine fin(itest)
c----------------------------------------------------------------------
c
c --- SMERGE     Version: 5.7.0     Level: 090511              FIN
c ---            J. Scire

c
c --- PURPOSE:  Run termination routine -- compute runtime,
c               write last day processed
c
c --- UPDATE
c --- V5.3 (030402) to V5.651 (090511)  (DGS)
c        - Reformat date reported at end of run
c
c --- V5.2-V5.3    030402  (DGS): Add list file unit number to JULDAY
c                                 (CALUTILS version), and fix unit
c                                 number given to GRDAY
c --- V5.1-V5.2    020828  (DGS): rdate, rdate2 changed to include
c                                 YYYY format for year (MM-DD-YYYY)
c
c --- INPUTS:
c          ITEST - integer - Flag indicating if execution is to
c                            include COMPUTATIONAL phase
c                            (ITEST = 1 to STOP program and skip
c                                       the COMPUTATIONAL phase
c                             ITEST = 2 to CONTINUE execution to
c                                       include computations)
c       Common block /DATEHR/
c          jyr, jjul, jhr
c       Common block /QA/
c          rdate, rtime, rcpu
c       Parameters: IO6, IOMESG
c
c --- OUTPUT:  none
c
c --- FIN called by:  MAIN
c --- FIN calls:      DATETM, JULDAY, DELTT, YR4C, FMT_DATE
c----------------------------------------------------------------------
c
c --- include parameters
      include 'params.smg'
c
      character*8 rtime2
      character*10 rdate2
      character*12 rdate12
c
      include 'datehr.smg'
      include 'qa.smg'
c
      write(iomesg,*)'TERMINATION PHASE'
c
c --- Write last day/hour processed
      if(ITEST.eq.2)then
c ---    Compute month & day from Julian day
         call GRDAY(io6,jyr,jjul,jmo,jday)
         write(io6,5)jyr,jmo,jday,jjul,jhr
5        format(//2x,'LAST DAY/HOUR PROCESSED:'/5x,'Year: ',i4,2x,
     1   'Month: ',i2,3x,'Day: ',i2,3x,'Julian day: ',i3,3x,'Hour: ',
     2   i2)
      else
c
c ---    TEST mode -- COMPUTATIONAL phase skipped
         write(io6,12)
12       format(/1x,13('----------')//1x,
     1   'Completion of test mode run -- run terminating ',
     2   'normally'//1x,13('----------'))
      endif
c
c --- get system date & time at end of run
      call datetm(rdate2,rtime2,rcpu)
c
c --- compute runtime
      read(rtime(1:2),10)ihr1
      read(rtime(4:5),10)imin1
      read(rtime(7:8),10)isec1
10    format(i2)
      t1=ihr1*3600.+imin1*60.+isec1
c
      read(rtime2(1:2),10)ihr2
      read(rtime2(4:5),10)imin2
      read(rtime2(7:8),10)isec2
      t2=ihr2*3600.+imin2*60.+isec2
c
      if(rdate.eq.rdate2)then
         delt=t2-t1
      else
         read(rdate(1:2),10)imo1
         read(rdate(4:5),10)iday1
         read(rdate(7:10),'(i4)')iyr1
         call julday(io6,iyr1,imo1,iday1,ijul1)

         read(rdate2(1:2),10)imo2
         read(rdate2(4:5),10)iday2
         read(rdate2(7:10),'(i4)')iyr2
         call julday(io6,iyr2,imo2,iday2,ijul2)

c ---    compute no. hours from beg. of first hour of run to
c ---    ending hour of ending day of the run
         call deltt(iyr1,ijul1,ihr1,iyr2,ijul2,ihr2,idelhr)

c ---    adjust for minutes and seconds
         delt=idelhr*3600.-imin1*60.-isec1+imin2*60.+isec2
      endif

c --- On the PC, the runtime and CPU time are the same
c --- (DATETM provides RCPU = 0.0 on the PC)
      if(rcpu.EQ.0.0)rcpu=delt

c --- Report current date
      rdate12=rdate2(1:10)//'  '
      call FMT_DATE(io6,'MM-DD-YYYY','DD-MMM-YYYY',rdate12)
      write(io6,1402)rtime2,rdate12,NINT(delt),NINT(rcpu)
1402  format(//2x,'End of run -- Clock time: ',a8/
     1         2x,'                    Date: ',a12//
     2         2x,'      Elapsed Clock Time: ',i12,' (seconds)'//
     3         2x,'                CPU Time: ',i12,' (seconds)')
c
      return
      end

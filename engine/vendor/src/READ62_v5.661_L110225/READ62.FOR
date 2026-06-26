c----------------------------------------------------------------------
c --- READ62 -- TD-6201 Upper Air Data Preprocessor
c----------------------------------------------------------------------
c
c --- READ62   Version: 5.661    Level: 110225                    MAIN
c
c     Copyright (c) 1995-2011 by Exponent, Inc.
c
c --- PURPOSE:
c      THIS PROGRAM READS A TD-6201 UPPER AIR FILE or an NCDC CD-ROM
c      upper air data file (FSL format), EXTRACTS DATA FOR PRESSURE 
c      LEVELS REQUESTED, AND CREATES A FORMATTED FILE FOR EDITING AND 
c      INPUT TO THE CALMET MODEL
c
c----------------------------------------------------------------------
c --- Model Change Bulletin Updates Included:           MCB-A (040716)
c----------------------------------------------------------------------
c
c --- UPDATES
c
c --- V 5.66  Level 100621  ====> V 5.661  Level 110225 (D.Strimaitis)
c     - Updated CALUTILS.FOR to version v2.58 (110225)
c        Add control file variable type 5 (character array) and
c        retain commas in string returned so that array of
c        variable values can be parsed
c       Modified: READIN, ALTONU, SETVAR
c
c --- V 5.651  Level 090511  ====> V 5.66  Level 100621 (D.Strimaitis)
c     - Fix typo in year argument of YR4 call for TD-6201 data
c     - Rearrange QA repair code placement
c       Modified: COMP
c     - Compute time differences in hours within SUBMISS for
c       substitution logic so that hr=24 and hr=0 convention is
c       treated the same
c       Modified: SUBMISS
c     - Reduce the allowed time difference between the substitute and
c       an observed sounding to 3 hours
c       Modified: SUBREP
c
c --- V 5.65  Level 080919  ====> V 5.651  Level 090511 (D.Strimaitis)
c     - CALUTILS from v2.56 Level 080407 to v2.571 Level 090511
c       Increase control file line length to 200 characters
c       Activate CPU clock using F95 system routine
c       Add routine to reformat a date string
c       New     : FMT_DATE
c       Modified: PARAMS.CAL, READIN, DATETM
c     - Reformat date reported to list file
c       Modified: FIN
c
c --- V 5.641  Level 080407  ====> V 5.65  Level 080919 (D.Strimaitis)
c     - Revise header records of UP.DAT file written with the time zone
c       for the specialized format that includes the station location
c     - Revise logic for reading the time zone record in the header of
c       the substitution UP.DAT file, when present
c       Modified:  WRHD, RDHDSUB
c     - Fix wrong format statement line number for reading time record
c       from the data section of the substitute sounding file
c       Modified:  RDSUB
c     - Replace null characters in lat/lon strings with blanks
c       Modified:  RDSTA
c
c --- V 5.64  Level 070327  ====> V 5.641  Level 080407 (D.Strimaitis)
c     - CALUTILS from v2.55 Level 070327 to v2.56 Level 080407
c       Control file entries in exponential notation were not correct
c       if decimal point was missing (2e-02 was read as 0.2e-02).
c       Modified: ALTONU
c
c --- 5.63  Level 061020  ====> V 5.64  Level 070327 (D.Strimaitis)
c     - CALUTILS from v2.54 Level 061020 to v2.55 Level 070327
c       Fixed format bug in subroutine BASRUTC for the case of
c       time zone zero (output string was 'UTC+0  0' instead of
c       'UTC+0000'
c       Modified:  UTCBASR, BASRUTC
c
c --- V 5.62  Level 060519  ====> V 5.63  Level 061020 (D.Strimaitis)
c     - Change output begin/end times to be equal, indicating that a
c       profile should be interpolated in time rather than treated as
c       an average.  Use end-time when assessing alignment of periods
c       such as when a substitute sounding is used.
c       Modified:  COMP, WRHD, RDSUB, SUBMISS, SUBREP
c
c     - Add starting and ending time (seconds) in /CONTROL/
c     - Add test for seconds out of range
c     - Process sec=3600 to sec=0 of next hour
c     - Hour 24 conversion sections must not be nested as ELSE-IF
c     - Remove 1-hr bump of start-time in version 2.1 control file
c       (it does not need to be "hr-ending") 
c       Modified:  CONTROL.R62
c                  SETUP, READCF, QAINP
c
c     - CALUTILS from v2.52 Level 060519 to v2.54 Level 061020
c       Allow negative increments in INCRS
c       Remove routine GLOBE1 (move to COORDLIB)

c
c --- V 5.61  Level 060309  ====> V 5.62  Level 060519 (D.Strimaitis)
c     - CALUTILS from v2.51 Level 051019 to v2.52 Level 060519
c       Variable names in control file are not processed correctly
c       if there are too many characters (including blanks) to the
c       left of the "=" sign (run stops in setup phase).
c       Modified:  READIN
c
c --- V 5.6  Level 041123  ====> V 5.61  Level 060309 (D.Strimaitis)
c     - Updated to CALUTILS V2.51 (051019) from V2.5 (041123)
c     - Filnames changed from c*70 to c*132 (for CALUTILS V2.3 and later)
c       Modified:  FILNAM.R62
c                  READCF, SETUP
c
c --- V 5.53 Level 040109  ====> V 5.6  Level 041123 (F.Robe)
c     1) Introduce new time format with beginning/ending times for each 
c        record, including seconds , as well as UTC time zone
c        (UP.DAT version 2.1)
c     2) Changes only to the input/output subroutines, not to the main
c        computational subroutines. Hourly (at most) records only-
c        Changes to subroutines WRHD,READCF,COMP,SUBMISS
c     3) Changes to READ62.INP (explicit beg/ending times with minutes/seconds)
c        However READ62 can still read old READ62.INP files (with
c        hour-ending beg/ending times (no seconds)
c
c --- V 5.52 Level 030709  ====> V 5.53 Level 040109 (D. Strimaitis)
c     1)  Remove lines using old limit of 79 levels from the TD-6201
c         section.  These lines caused 1 or 2 dummy reads leading to
c         "skipped soundings" in the output file. 
c         This does not affect FSL data.
c
c --- V 5.51 Level 030528  ====> V 5.52 Level 030709 (D. Strimaitis)
c     1)  Fix type assignment for LCFILES in READCF
c
c --- V 5.5 Level 030402  ====> V 5.51 Level 030528 (D. Strimaitis)
c     1)  Updated CALUTILS (Version 2.2, Level 030528)
c
c --- V 5.4 Level 021024  ====> V 5.5 Level 030402 (D. Strimaitis)
c     1)  Updated CALUTILS (Version 2.1, Level 030402)
c     2)  New header for output data file (UP.DAT)
c             ENVPOP=0  Revised UP.DAT header
c             ENVPOP=1  Revised UP.DAT header with station locations
c     3)  Use WBAN ID in FSL file if available, otherwise use WMO
c     4)  Fix N,S,E,W logic for WEB FSL format
c
c --- V 5.3 Level 020828  ====> V 5.4 Level 021024 (D. Strimaitis)
c     1)  Fixed bug in extrapolation repair code (bug had allowed
c         extrapolation from levels below user's minimum level)
c
c --- V 5.2 Level 020805  ====> V 5.3 Level 020828 (D. Strimaitis)
c     1)  Updated CALUTILS (Version 1.1, Level 020828)
c
c --- V 5.1 Level 020614  ====> V 5.2 Level 020805 (D. Strimaitis)
c     1)  Change FSL header format for new WEB structure
c     2)  Add parameter ENVPOP to params.r62 and use this to toggle
c         environment configurations (these will evolve in time):
c             ENVPOP=0  UP.DAT header as in 1/2000 Users Guide
c             ENVPOP=1  UP.DAT header with draft station locations
c                       introduced in V 5.1 Level 020330
c
c --- V 5.1 Level 020330  ====> V 5.1 Level 020614 (D. Strimaitis)
c     1)  Allow station ID to be up to 8 characters
c
c --- V 5.0 Level 011003  ====> V 5.1 Level 020330 (D. Strimaitis)
c     1)  Accept variable record length version of TD6201 format
c     2)  New header format for UP.DAT:  include READ62 version, level,
c         station name, ID, and LAT/LON
c         (Station information is read from file headers)
c     3)  Add repair option to extrapolate missing data to either the
c         surface and/or the top pressure level
c     4)  Add option to substitute alternate sounding profiles when
c         observed sounding is deficient (from UP.DAT format file)
c
c --- V 4.0 Level 010315  ====> V 5.0 Level 011003 (D. Strimaitis)
c     1)  Restructure inputs for CALPUFF system control file
c     2)  Restructure main program as subroutine COMP
c     3)  Place system-wide utilities into an include module
c           (calutil.for)
c
c --- V 4.0 Level 990228  ====> V 4.0 Level 010315 (D. Strimaitis)
c     1)  Read full year from input records and enforce YYYY format
c         for all years (Y2k)
c
c --- V 4.0 Level 980304  ====> V 4.0 Level 990228 (J. Scire)
c     1) Modified format statements 6060/6200/6065/6205 to allow more 
c        than 99 levels to be written in data header record
c     2) Allow wind speed to be written in xxx.x format in m/s when
c        using comma-delimited output file.  Add comma after wind
c        speed field in data record (except after last value).
c        Eliminate truncation of wind speed data when using FSL data 
c        input.
c     3) Perform check on no. levels to ensure array dimensions are
c        not exceeded (FSL formatted data).
c     4) Fix bug preventing elimination of levels when LTEMP=T (caused by
c        change of missing value indicator from 99.9 to 999.9).
c     5) Fix bug allowing conversion of temperature to deg. K when
c        using td-6201 data.
c     6) Update version number in function GOOD.
c     7) Add check for header entry with no data records (FSL)
c     8) Perform QA checks & produce warnings relevent for CALMET
c        - flag data missing at bottom or top of sounding
c        - check that first layer is at ground
c        - flag if pressure increases with decreasing level number
c        - flag if elevation decreases with increasing level number
c        - flag WD < 0 or WD > 360 degrees
c        - flag WS < 0
c        - flag T < TMIN, T > TMAX (TMIN set at 175 K (-99 C, -146 F),
c                                   TMAX set at 322 K (120 F))
c        - flag P < PMIN, P > PMAX in final sounding
c        - check for missing height (FSL format uses 32767 as missing
c                                    value indicator)
c        - NOTE: delta pressure & elevation checks exclude layers with
c                missing data
c     9) Use parameter to dimension max. number levels & unit numbers
c    10) Write correct number of levels in original sounding (MLEV)
c    11) Fix bug where missing height was assigned to -990 instead of 9999.
c    12) Use UP.DAT convention of -99.9 for missing pressures on output
c
c --- V 4.0 Level 970730  ====> V 4.0 Level 980304 (R. O'Neal, E. Insley)
c     1) Modified WRITE statement 6010 to reflect the current version
c        (was still writing out version 970131).
c     2) Corrected check for missing days which then get sent to
c        subroutine DELTT.  First julian day argument (JDAY2) passed in
c        DELTT changed to JDAY1.  This corrects problem of READ62
c        failing to pick up missing sounding time under certain
c        circumstances.
c     3) Added a check for soundings which get written with missing or invalid
c        heights.  They are still written with missing heights, but a
c        message is written to the list file to alert the user.
c
c --- V 4.0 Level 970131  ====> V 4.0 Level 970730 (E. Insley)
c     1) Modified to allow the data to span from one year to the next
c        without getting a "MISSING DAYS" message (unless, of course
c        there really is a missing day.
c     2) Corrected check for missing/duplicate soundings.  It now uses
c        Subr. DELTT to compute the delta time between soundings and only
c        writes messages when appropriate.
c
c --- V 4.0 Level 961113  ====> V 4.0 Level 970131 (E. Insley, V. Tino)
c     1) Modified to use up to 24 user-specified observation hours,
c        thereby allowing any upper air soundings in raw data file be
c        retrieved by READ62 instead of just 00 and 12 GMT.
c        (EMI - 12/13/96)
c
c     2) Added check if all levels bad (only 0 or 1 good level), skip
c        sounding.
c
c     3) Modified check for missing hours to reflect acceptable
c        user-input observation times.  Uses an algorithm based on
c        12 hour difference.  Difference between 2 consecutive obs.
c        must be less than or equal to 12 hours.
c
c --- V 4.0 Level 961025  ====> V 4.0 Level 961113 (E. Insley, J. Scire)
c     1) Changed STATUS to 'UNKNOWN' in opens for output files
c        (READ62.LST, UP.DAT)
c     2) Modified UP.DAT to include a flag indicating the file type
c        on the first header record (i.e., IFMT=1 for slash-delimited
c        file, IFMT=2 for comma-delimited file)
c     3) Added option to read filenames from the control file
c        (READ62.INP) -- Filenames read: READ62.LST, TD6201.DAT or
c        NCDC_U.DAT, and UP.DAT.
c     4) Added QA checks on values of control file input parameters
c     5) Updated comments in MAIN program
c     6) Eliminated some old, inactive code
c
c --- V 3.0 Level 941215  ======> V 4.0 Level 961025   (E. Insley)
c     1.) Added option to output comma delimited UP.DAT file which can
c         be read using a free format, or / delimited soundings as
c         originally output
c     2.) Fixed location of where the 'MISSING DAYS' and 'MISSING/DUPLICATE
c         SOUNDING' messages are printed. Now they are printed where the
c         missing sounding should be located instead of after the next
c         valid sounding.
c     3.) Changed format at end of program to read 'LAST YR,DAY PROCESSED'
c         and separated the YR and DAY by a comma (e.g., 83,365)
c
c   Modified - VRT - 8/15/95 -
c     Error in processing CD-ROM data.  The
c     1000mb level was included in the sounding even if the surface
c     level was above 1000mb (i.e. 990mb).  In this case, we want any
c     sounding level that is out of order (p increases with Z) to be
c     deleted from the sounding.
c
c     Error in processing missing temperature data.  Conversion of temp
c     from Celsius to Kelvin occurs within the write statement.  This
c     produces an error when the temp in C is missing (99.9). The temp
c     output to the UP.DAT file should stay at 99.9 but all nonmissing
c     temps should be converted to Kelvin.
c
c     Error in reading CD-ROM data.  Calmet needs the 5 digit WBAN number
c     to be compared to the number given in the CALMET.INP file (Group 8)
c     Changed to read and write the WBAN # instead of the 3-letter WMO code.
C                                                                       R6200380
C DETAILS OF TD-6201 CONTENT:                                           R6200390
C                                                                       R6200400
C       HEADER INFORMATION FOR EACH SOUNDING TIME:                      R6200410
C                                                                       R6200420
C      STNID          STATION IDENTIFICATION                            R6200430
C      LAT            LATITUDE -- THE STATION LATITUDE IN DEG AND MIN,  R6200440
C                       FOLLOWED BY 'N' OR 'S'                          R6200450
C      LON            LONGITUDE-- THE STATION LONGITUDE IN DEG AND MIN, R6200460
C                       FOLLOWED BY 'E' OR 'W'                          R6200470
C      YEAR, MONTH, DAY, HOUR  -- THE SCHEDULED TIME OF THE OBSERVATION R6200480
C      NUMLEV         NUMBER OF REPEATING GROUPS -- THIS REPRESENTS     R6200490
C                       THE NUMBER OF DATA LEVELS FOUND IN THE CURRENT  R6200500
C                       OBSERVATION (79 IS THE MAXIMUM NUMBER STORED)   R6200510
C                                                                       R6200520
C       DATA FOR EACH NUMLEV PRESSURE LEVEL:                            R6200530
C                                                                       R6200540
C      QIND           LEVEL-QUALITY-INDICATOR -- DENOTES THE RESULTS OF R6200550
C                       ANY QUALITY CONTROLS APPLIED TO THIS LEVEL (THISR6200560
C                       IS USED IN THIS PROGRAM)                        R6200570
C      ETIME          THE ELAPSED TIME SINCE THE RELEASE OF THE SOUNDINGR6200580
C                       IN MINUTES AND TENTHS (IGNORED HERE)            R6200590
C      PRES           ATMOSPHERIC PRESSURE AT THE CURRENT LEVEL (READ INR6200600
C                       AS MILLIBARS)                                   R6200610
C      HGT            GEOPOTENTIAL HEIGHT OF THE CURRENT LEVEL IN METERSR6200620
C      TEMP           THE FREE AIR TEMPERATURE AT THE CURRENT LEVEL IN  R6200630
C                       DEGREES AND TENTHS CELSIUS.                     R6200640
C      RH             THE RELATIVE HUMIDITY AT THE CURRENT LEVEL IN %   R6200650
C      WD             DIRECTION OF THE WIND AT THE CURRENT LEVEL IN DEG R6200660
C      WS             SPEED OF THE WIND IN WHOLE METERS PER SECOND.     R6200670
C      TIMEF,PRESF,HGTF,TEMPF,RHF,WINDF  --  QUALITY CONTROL FLAGS      R6200680
C                       (USED HERE)                                     R6200690
C      TYPLEV         TYPE OF LEVEL FLAG (IGNORED HERE)                 R6200700
C                                                                       R6200710
C                                                                       R6200720
C      EXTERNAL FUNCTION: GOOD (INTEGER)                                R6200730
C                                                                       R6200740
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.r62'
c --- Include common blocks
      include 'qa.r62'
c
c --- Set version and level number of program (stored in /QA/ and
c --- checked against values set in PARAMS.R62)
      ver='5.661'
      level='110225'
c
c --- SETUP PHASE -- read control file information
      call SETUP
c
c --- COMPUTATIONAL PHASE -- process data files
      call COMP
c
c --- TERMINATION PHASE -- program termination functions
      call FIN
c
      stop
      end
c----------------------------------------------------------------------
      BLOCK DATA
c----------------------------------------------------------------------
c
c --- READ62    Version: 5.661         Level: 011003        BLOCK DATA
c               D. Strimaitis, Earth Tech, Inc.
c
c----------------------------------------------------------------------
c
c --- Include parameter statements
      include 'params.r62'
c
c --- Include common blocks
      include 'control.r62'
      include 'filnam.r62'
      include 'qa.r62'
      include 'station.r62'

c --- FILNAM common block
      data runinp/'read62.inp'/,runlst/'read62.lst'/,
     1     indat/'sounding.dat'/,updat/'up.dat'/,subdat/'subsound.dat'/
c --- FILLOG common block
      data lcfiles/.true./

c --- CONTROL common block
      data lht/.false./, ltemp/.false./, lwd/.false./, lws/.false./
      data ifmt/2/
      data pstop/700.0/
      data isub/0/
      data lxtop/.false./, lxsfc/.false./
      data pvtop/700./, zvsfc/200./

c --- QA common block
      data model/'READ62      '/

c --- STATION common block
      data istz/0/
      data cname/'----'/

      end

c----------------------------------------------------------------------
c --- BRING IN CALPUFF SYSTEM UTILITY SUBROUTINES
      include 'calutils.for'
c----------------------------------------------------------------------

c-----------------------------------------------------------------------
      subroutine setup
c-----------------------------------------------------------------------
c
c --- READ62    Version: 5.661         Level: 061020             SETUP
c               D. Strimaitis, Earth Tech, Inc.
c
c PURPOSE:     SETUP calls routines to read and check the control data
c              provided, it reports the control data to the list file,
c              and it opens the data files if inputs are valid.
c
c --- UPDATES:
c
c --- V5.61 Level 060309 ====> V5.63 Level 061020  (D.Strimaitis)
c     - Report starting/ending times to the second
c
c --- V 5.1 Level 020330 ====> V5.61 (060309)  (D.Strimaitis)
c     - Change filenames from c*70 to c*132
c
c --- INPUTS:
c
c ---    Common block /QA/ variables:
c           VER, LEVEL
c ---    Common block /FILNAM/ variables:
c           runlst,indat,subdat,updat,
c           lcfiles
c ---    Common block /CONTROL/ variables:
c           IBYR,IBMO,IBDY,IBHR,IEYR,IEMO,IEDY,IEHR,
c           ibsec,iesec,ibjul,iejul,
c           lht,ltemp,lwd,lws,lxtop,lxsfc,
c           jdat,ifmt,isub,
c           pstop,pvtop,zvtop,itimesec
c
c        Parameters: IO5, IO6, IO8, IO9, IO18 IOMESG, MVER, MLEVEL
c
c --- OUTPUT:
c
c ---    Common block /QA/ variables:
c           rcpu,rtime,rdate
c
c ---    Common block /FILNAM/ variables:
c           runinp
c
c CALLING ROUTINES:   MAIN
c
c EXTERNAL ROUTINES:  DATETM, COMLINE, READCF, QAINP, RDSTA
c-----------------------------------------------------------------------
c --- Include file of parameters and commons
      include 'params.r62'
      include 'control.r62'
      include 'filnam.r62'
      include 'qa.r62'

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
10       format(/1x,'ERROR in SUBR. SETUP -- The READ62 version ',
     1   'and level numbers do not match those in the parameter file'/
     2   5x,'    Model Code - Version: ',a12,' Level: ',a12/
     3   5x,'Parameter File - Version: ',a12,' Level: ',a12)
         stop
      endif

c --- Read control file (open list file)
      call READCF

c --- Write control data to list-file
      WRITE(io6,6020)IBYR,IEYR,ibmo,iemo,ibdy,iedy,
     &               IBHR,IEHR,ibsec,iesec
6020  FORMAT(/1x,'STARTING DATE:',
     1 16X,'ENDING DATE:'//,
     1 16X,'YEAR = ',I4,18X,'YEAR = ',I4/,
     2 15X,'MONTH = ',I4,17X,'MONTH = ',I4/,
     2 17X,'DAY = ',I4,19X,'DAY = ',I4/,
     3 16X,'HOUR = ',I4,' (UTC)',12X,'HOUR = ',I4,' (UTC)',/
     4 14X,'SECOND = ',I4,' (UTC)',10X,'SECOND = ',I4,' (UTC)')
      WRITE(io6,6030)PSTOP
6030  FORMAT(//1x,'PRESSURE LEVELS EXTRACTED:'//1x,20X,'SURFACE',
     1 ' TO  ',F5.0,' MB')
      write(io6,6031)jdat,ifmt
6031  format(//1x,'INPUT FILE FORMAT (1=TD6201,2=NCDC CD-ROM): ',i3,
     1 /1x,'OUTPUT FILE FORMAT (1=/ DELIMITED,2=COMMA DELIMITED): ',i3)

      if(isub.EQ.0) then
         write(io6,6032)
      else
         write(io6,6033) isub
      endif
6032  format(//1x,'ALT. SOUNDING FILE FOR SUBSTITUTIONS IS NOT USED ')
6033  format(//1x,'ALT. SOUNDING FILE (1=/ DELIMITED,2=COMMA',
     &            ' DELIMITED): ',i3)

      WRITE(io6,6040)LHT,LTEMP,LWD,LWS
6040  FORMAT(//1X,'DATA LEVEL ELIMINATED IF HEIGHT MISSING ? ',8X,L1/
     1 /1X,'DATA LEVEL ELIMINATED IF TEMPERATURE MISSING ? ',3X,L1/
     2 /1X,'DATA LEVEL ELIMINATED IF WIND DIRECTION MISSING ? ',L1/
     3 /1X,'DATA LEVEL ELIMINATED IF WIND SPEED MISSING ? ',4X,L1)

      WRITE(io6,6041)LXTOP,PVTOP,LXSFC,ZVSFC
6041  FORMAT(//1X,'MISSING PROFILE DATA EXTRAPOLATED TO TOP ? ',8X,L1/
     1 /1X,'Last valid data must be above pressure (mb): ',3X,f7.1/
     2 /1X,'MISSING PROFILE DATA EXTRAPOLATED TO SURFACE ? ',4X,L1/
     3 /1X,'First valid data must be below height (m AGL): ',3X,f7.1)

      write(io6,6042) runinp,indat,updat,runlst
6042  format(//1x,'FILENAMES: '/
     1 5x,'Control file:          ',a132/
     2 5x,'Input upper air file:  ',a132/
     3 5x,'Output upper air file: ',a132/
     4 5x,'Output list file:      ',a132)

c --- Substitution sounding file (conditional)
      if(isub.GT.0) then
         write(io6,6043) subdat
6043     format(5x,'Substitution UP.DAT:   ',a132)
     1    
      endif

c --- Apply QA checks
      call QAINP

c --- Open the upper air input data file (TD6201.DAT or NCDC-UP.DAT)
      if(jdat.EQ.1) then
c         open(io8,FILE=indat,status='old',form='formatted',
c     1        recl=2876)
         open(io8,FILE=indat,status='old',form='formatted',
     1        recl=7232)
      else
         open(io8,FILE=indat,status='old')
      endif

c --- Open a second upper air input data file for soundings used as
c --- substitutions (UP.DAT format) and read the header
c --- of the substitution file
      if(isub.GT.0) then
         open(io18,FILE=subdat,status='old')
         call RDHDSUB(io18,dataversub)
      endif

c --- Get station information from input data file
      call RDSTA

c --- Open the output UP.DAT file
      open(io9,file=updat,status='unknown')

      return
      end
c----------------------------------------------------------------------
      subroutine readcf
c----------------------------------------------------------------------
c
c --- READ62    Version: 5.661         Level: 061020            READCF
c               D. Strimaitis, Earth Tech, Inc.
c
c --- PURPOSE:  Read the control file containing the file names of
c               the input and output files of the run, and associated
c               control variables.  Open list file for output.
c
c --- UPDATES:
c --- V 5.61 Level 060309 ====> V 5.63 Level 061020  (D.Strimaitis)
c     - Move QA on input times to QAINP
c --- V 5.6 Level 041123 ====> V 5.61 Level 060309  (D.Strimaitis)
c     - Filenames from c*70 to c*132 for CALUTILS V2.3 and later
c --- V 5.52 Level 030709 ====> V5.6 (041123)  (F.Robe)
c     - Read in first line of Input file and extract input file version number
c     - Computation period defined in READ62.INP with explicit  
c       beginning/ending times including seconds 
c     - Old input files (without seconds) can still be read in
c     - Convert those explicit times back to hour-ending times (ibyr,ieyr,etc)
c
c --- V 5.5 Level 020330  ====> V 5.52 Level 030709 (D. Strimaitis)
c           - Fix type assignment for LCFILES
c
c --- INPUTS:
c
c ---    Common block /QA/ variables:
c           VER, LEVEL
c
c ---    Common block /FILNAM/ variables:
c           runinp
c
c        Parameters: IO5, IO6, IOMESG, MXLEV
c
c --- OUTPUT:
c
c ---    Common block /FILNAM/ variables:
c           runlst,indat,subdat,updat,
c           lcfiles
c ---    Common block /CONTROL/ variables:
c           IBYR,IBMO,IBDY,IBHR,IEYR,IEMO,IEDY,IEHR,
c           ibsec,iesec,ibjul,iejul,
c           lht,ltemp,lwd,lws,lxtop,lxsfc,
c           jdat,ifmt,isub,
c           pstop,pvtop,zvtop
c
c --- READCF called by:  SETUP
c --- READCF calls:      READIN, FILCASE, JULDAY
c----------------------------------------------------------------------
c
c --- Include parameter statements and commons
      include 'params.r62'
      include 'params.cal'
c
c --- Include common blocks
      include 'control.r62'
      include 'filnam.r62'
      include 'qa.r62'
c
c --- Local variables
      character*4 ctemp(132,4)
      character*12 cvdic(mxvar,2)
      integer ivleng(mxvar,2),ivtype(mxvar,2)
      logical lecho
      character*16 inputset,inputver
      character*64 inputmod

c --- Initialize local variables
      data lecho/.false./
      data names/4/

c --- Set Dictionary
C --- 10 new dictionary variables for explicit beginning/ending times (041123)
      data cvdic/
     a  'RUNLST','INDAT','SUBDAT','UPDAT','LCFILES', 55*' ',
     b  'IBYR','IBMO','IBDY','IBHR','IBSEC','IEYR','IEMO',
     b  'IEDY','IEHR','IESEC',
     b  'JDAT','IFMT','PSTOP','LHT','LTEMP','LWD','LWS',
     b  'ISUB','PVTOP','ZVSFC','LXTOP','LXSFC', 38* ' '/

      data ivleng/
     a  4*132,1, 55*0,
     b  22*1, 38*0/

c --- Variable types (ivtype) are coded as:
c          0 = null
c          1 = real
c          2 = integer
c          3 = logical
c          4 = character
      data ivtype/
     a  4*4,3, 55*0,
     b  12*2,1,4*3,2,2*1,2*3, 38*0/



c --- New format: first line inludes dataset types, version number
c --- and description (starts at 2.1)
      read(io5,'(2a16,a64)') inputset,inputver,inputmod
      

c ------------------
c --- Input Group 0
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
     1 ctemp(1,1),ctemp(1,2),ctemp(1,3),ctemp(1,4),lcfiles,
     2 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     3 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     4 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     5 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     6 idum,idum,idum,idum,idum,idum,idum)

c --- Prepare any filenames included in the I/O file by erasing
c --- the default filename set above
      if(ctemp(1,1)(1:1).ne.' ')runlst=' '
      if(ctemp(1,2)(1:1).ne.' ')indat=' '
      if(ctemp(1,3)(1:1).ne.' ')subdat=' '
      if(ctemp(1,4)(1:1).ne.' ')updat=' '

c --- Transfer the char*4 data into the char*132 variables
      do j=1,132
         if(ctemp(j,1)(1:1).ne.' ')runlst(j:j)=ctemp(j,1)(1:1)
         if(ctemp(j,2)(1:1).ne.' ')indat(j:j)=ctemp(j,2)(1:1)
         if(ctemp(j,3)(1:1).ne.' ')subdat(j:j)=ctemp(j,3)(1:1)
         if(ctemp(j,4)(1:1).ne.' ')updat(j:j)=ctemp(j,4)(1:1)
      enddo

c --- Convert the file names to the proper case
      call FILCASE(lcfiles,runlst)
      call FILCASE(lcfiles,indat)
      call FILCASE(lcfiles,subdat)
      call FILCASE(lcfiles,updat)

c --- Open listfile
      open(io6,file=runlst,status='unknown')

c --- Write banner to list file
      write(io6,5) ver,level
5     format(///,26x,'READ62 OUTPUT SUMMARY',/,19x,'VERSION:  ',A12,
     1       ' LEVEL:  ',A12///)

c -----------------
c --- Input Group 1
c -----------------
C --- Initialize dates variable to check whether old or new input file 
      ibsec=-9999
      iesec=-9999
 
      call readin(cvdic(1,2),ivleng(1,2),ivtype(1,2),io5,io6,lecho,
     1 IBYR,IBMO,IBDY,IBHR,IBSEC,IEYR,IEMO,IEDY,IEHR,IESEC,
     2 JDAT,IFMT,PSTOP,LHT,LTEMP,LWD,LWS,
     3 ISUB,PVTOP,ZVSFC,LXTOP,LXSFC,
     4 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     5 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     6 idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,idum,
     7 idum,idum)

      if(inputver.EQ.'2.1') then
c ---    New input format with beginning/ending times including seconds
c ---    check that new variables IBSEC,IESEC are there
         if( (ibsec.eq.-9999).or.(iesec.eq.-9999) ) then
            write(io6,*)'READ62.INP dataset version 2.1'
            write(io6,*)'Requires beginning/ending seconds: IBSEC,IESEC'
            write(io6,*)'Update your input file - STOP'
            STOP 'Error in control file - check LST file -STOP -'
         endif
      else if ((ibsec.ne.-9999).or.(iesec.ne.-9999)) then
c ---    Should be old input file with hour ending times without seconds
         write(io6,*)'READ62.INP dataset version prior to 2.1'
         write(io6,*)'For READ62.INP version 2.1 add 1st line header'
         write(io6,*)'Update your input file - STOP'
         STOP 'Error in control file - check LST file -STOP - '
      endif
c --- Clear -9999 now that file format has been assessed
      if(ibsec.EQ.-9999) ibsec=0
      if(iesec.EQ.-9999) iesec=0


      return
      end
c----------------------------------------------------------------------
      subroutine qainp
c----------------------------------------------------------------------
c
c --- READ62    Version: 5.661         Level: 061020             QAINP
c               D. Strimaitis, Earth Tech, Inc.
c
c --- PURPOSE:  QA the control file information
c
c --- UPDATES:
c --- V 5.1 Level 020330 ====> V 5.63 Level 061020  (D.Strimaitis)
c     - Move QA on input times from READCF
c     - Add test for seconds out of range
c     - Process sec=3600 to sec=0 of next hour
c     - Hour 24 conversion sections must not be nested as ELSE-IF
c     - Remove 1-hr bump of start time in version 2.1 control file
c       to make it hr-ending
c
c --- INPUTS:
c
c ---    Common block /CONTROL/ variables:
c           IBYR,IBMO,IBDY,IBHR,IEYR,IEMO,IEDY,IEHR,
c           ibsec,iesec,ibjul,iejul,
c           jdat,ifmt,pstop
c
c        Parameters: IO6
c
c --- OUTPUT:   none
c
c --- QAINP called by:  SETUP
c --- QAINP calls:      QAYR4, YR4
c----------------------------------------------------------------------
c
c --- Include parameter statements and commons
      include 'params.r62'
c
c --- Include common blocks
      include 'control.r62'
c
c --- Local variables
      logical lerrcf

c --- Initialize local variables
      data lerrcf/.false./

c --- Special Y2K QA on starting year of simulation
      call QAYR4(io6,ibyr,0,ierr)
      if(ierr.NE.0) then
         write(io6,*) 'QAINP:  Y2K Error in Input Group 1'
         lerrcf=.TRUE.
      endif

c --- Make sure ending year is YYYY (Y2K)
      call YR4(io6,ieyr,ierr)
      if(ierr.NE.0) then
         write(io6,*) 'QAINP:  Y2K Error in Input Group 1'
         lerrcf=.TRUE.
      endif

      call JULDAY(io6,ibyr,ibmo,ibdy,ibjul)
      call JULDAY(io6,ieyr,iemo,iedy,iejul)

c --- Check for proper time entries
      if((ibsec.GT.3600) .OR. (iesec.GT.3600) .OR. 
     &   (ibsec.LT.0) .OR. (iesec.LT.0)) then
         write(io6,*)'IBSEC and IESEC must be between 0 and 3600'
         lerrcf=.TRUE.
      endif
      if((ibhr.GT.24) .OR. (iehr.GT.24) .OR.
     &   (ibhr.LT.0) .OR. (iehr.LT.0)) then
         write(io6,*)'IBHR and IEHR must be between 0 and 24'
         lerrcf=.TRUE.
      endif

c --- Condition hour fields
      if(ibhr.eq.24) then
         ibhr=0
         call INCR(io6,ibyr,ibjul,ibhr,24)
         call GRDAY(io6,ibyr,ibjul,ibmo,ibdy)
      endif
      if(iehr.eq.24) then
         iehr=0
         call INCR(io6,ieyr,iejul,iehr,24)
         call GRDAY(io6,ieyr,iejul,iemo,iedy)
      endif

c --- Condition seconds fields
      if(ibsec.EQ.3600) then
         nhrinc=1
         call INCR(io6,ibyr,ibjul,ibhr,nhrinc)
         call GRDAY(io6,ibyr,ibjul,ibmo,ibdy)
         ibsec=0
      endif
      if(iesec.EQ.3600) then
         nhrinc=1
         call INCR(io6,ieyr,iejul,iehr,nhrinc)
         call GRDAY(io6,ieyr,iejul,iemo,iedy)
         iesec=0
      endif

c --- Check that ending date is after starting date
      iedathr=ieyr*100000+iejul*100+iehr
      ibdathr=ibyr*100000+ibjul*100+ibhr
      if(ibdathr.GT.iedathr)then
         write(io6,*)
         write(io6,*) 'QAINP:  Error in Input Group 1'
         write(io6,*) 'Starting date/time is after ending date/time'
         write(io6,*) 'IBDATHR,IBSEC = ',ibdathr,ibsec
         write(io6,*) 'IEDATHR,IESEC = ',iedathr,iesec
         lerrcf=.TRUE.
      elseif(ibdathr.EQ.iedathr .AND. ibsec.GT.iesec)then
         write(io6,*)
         write(io6,*) 'QAINP:  Error in Input Group 1'
         write(io6,*) 'Starting date/time is after ending date/time'
         write(io6,*) 'IBDATHR,IBSEC = ',ibdathr,ibsec
         write(io6,*) 'IEDATHR,IESEC = ',iedathr,iesec
         lerrcf=.TRUE.
      endif

c --- Check for an invalid value of JDAT (input data type)
      if(jdat.NE.1 .AND. jdat.NE.2)then
         write(io6,*)
         write(io6,*) 'QAINP:  Error in Input Group 1'
         write(io6,*) 'JDAT out of range       = ',jdat
         write(io6,*) 'JDAT must be 1 (for TD-6201 file) or ',
     1                '2 (for NCDC FSL file)'
         lerrcf=.TRUE.
      endif

c --- Check for an invalid value of IFMT (input data format)
      if(ifmt.NE.1 .AND. ifmt.NE.2)then
         write(io6,*)
         write(io6,*) 'QAINP:  Error in Input Group 1'
         write(io6,*) 'IFMT out of range       = ',ifmt
         write(io6,*) 'IFMT must be 1 (for slash-delimited file) or ',
     1                '2 (for comma-delimited file)'
         lerrcf=.TRUE.
      endif

c --- Check for an invalid value of PSTOP (mb)
      if(pstop.GT.1500.0 .OR. pstop.LT.0.0)then
         write(io6,*)
         write(io6,*) 'QAINP:  Error in Input Group 1'
         write(io6,*) 'PSTOP(mb) out of range = ',pstop
         lerrcf=.TRUE.
      endif

c --- Check for an invalid value of ISUB (substitute option file)
      if(isub.NE.0 .AND. isub.NE.1 .AND. isub.NE.2)then
         write(io6,*)
         write(io6,*) 'QAINP:  Error in Input Group 1'
         write(io6,*) 'ISUB out of range       = ',isub
         write(io6,*) 'ISUB must be 0 (NOT USED) or ',
     1                '1 (for TD-6201 file) or ',
     2                '2 (for NCDC FSL file)'
         lerrcf=.TRUE.
      endif
c --- Conditional checks on repair parameters
      if(LXTOP) then
c ---    Check for an invalid value of PVTOP (mb)
         if(pvtop.GT.1500.0 .OR. pvtop.LT.pstop)then
            write(io6,*)
            write(io6,*) 'QAINP:  Error in Input Group 1'
            write(io6,*) 'PVTOP(mb) out of range = ',pvtop
            lerrcf=.TRUE.
         endif
      endif
      if(LXSFC) then
c ---    Check for an invalid value of ZVSFC (m AGL)
         if(zvsfc.GT.5000.0 .OR. zvsfc.LE.10.0)then
            write(io6,*)
            write(io6,*) 'QAINP:  Error in Input Group 1'
            write(io6,*) 'ZVSFC (m) out of range = ',zvsfc
            lerrcf=.TRUE.
         endif
      endif

c --- STOP now if error exists in the control file
      if(LERRCF) then
         write(*,*)'ERRORS are found in the CONTROL file'
         write(*,*)'Review messages written to the LIST file'
         stop
      endif

      return
      end
c----------------------------------------------------------------------
      subroutine comp
c----------------------------------------------------------------------
c
c --- READ62    Version: 5.661         Level: 100621              COMP
c ---           J. Scire, D. Strimaitis
c
c --- PURPOSE:  Main computational routine that reads soundings,
c               reports QA failures, and writes ouput data file.
c
c --- UPDATES:
c --- V 5.63 Level 061020  ====> V 5.66 Level 100621  (DGS)
c           - Fix typo for year in YR4 argument (TD-6201)
c           - Re-arrange code for QA repairs
c --- V 5.6 Level 041123  ====> V 5.63 Level 061020  (DGS)
c           - Report sounding times to the second
c             (at this time, all seconds explicitly ZERO)
c --- V 5.53 Level 040109  ====> V 5.6 Level 041123  (F.Robe)
c           - New output format (2.1) with explicit beginning/ending
c             times (includes seconds)
c           - Update WRHD arguments (new header)
c --- V 5.52 Level 030709  ====> V 5.53 Level 040109 (D. Strimaitis)
c           - Remove old limit of 79 levels from TD-6201 section
c --- V 5.4 Level 021024  ====> V 5.5 Level 030402 (D. Strimaitis)
c           - Update WRHD arguments (new header)
c           - Use WBAN ID in FSL file if available, otherwise use WMO
c --- V 5.2 Level 020805  ====> V 5.4 Level 021024 (D. Strimaitis)
c           - Fix bug in setting minimum level at which valid data
c             are found (bug resulted in using only temperature)
c --- V 5.1 Level 020614  ====> V 5.2 Level 020805 (D. Strimaitis)
c           - Change FSL header format for new WEB structure
c           - Remove io9 from WRHD call (params.r62 added to WRHD)
c --- V 5.1 Level 020330  ====> V 5.1 Level 020614 (D. Strimaitis)
c           - Allow station ID to be up to 8 characters
c --- V 5.0 Level 011003  ====> V 5.1 Level 020330 (D. Strimaitis)
c           - Allow either fixed or variable-length record TD6201 file
c           - Allow for profile repair and substitution
c
c --- INPUTS:
c       Parameters: IO6, IO8, IO9, IOMESG, MXLEV
c
c --- OUTPUT:  none
c
c --- COMP called by:  MAIN
c --- COMP calls:      YR4, DELTT, GOOD, WRHD, SUBMISS, SUBREP, RDSUB
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.r62'

c --- Include common blocks
      include 'control.r62'
      include 'qa.r62'
      include 'station.r62'
c
      REAL HEIGHT(mxlev),HIGHT(mxlev)
      REAL APRES(mxlev),ATEMP(mxlev),PRES(mxlev),TEMP(mxlev)
      INTEGER MON(12),LMON(12),YEAR,MONTH,DAY,HOUR,SECOND,GOOD,yearm1
      INTEGER WS(mxlev),AWS(mxlev),WD(mxlev),AWD(mxlev),RH,NCDCMON2(12)
      INTEGER WS1(mxlev)
c
      CHARACTER*32 JUNK
      CHARACTER*3 NCDCMON1(12),MONTHC
      CHARACTER*2 WSUNIT
      CHARACTER*1 LATA,LONA,QIND(mxlev),TIMEF(mxlev),PRESF(mxlev),
     1  HGTF(mxlev),TEMPF(mxlev),RHF,WINDF(mxlev),TYPLEV
      character*1 ccomma
c
      real xPRES(mxlev),xHEIGHT(mxlev),xTEMP(mxlev)
      real xWS1(mxlev),xaws1(mxlev)
      integer xWD(mxlev)
      integer iws1(mxlev)

c --- Declare local logicals for identifying problems with soundings
      logical lqamb,lqabot,lqatop,lqafirst,lqapht,lqazht,lqaelev
      logical lqawd,lqaws,lqat,lqap
      logical lrmb,lrbot,lrtop,lrepair

c
      common/tmp1/etime,rh,alat,alon
      common/tmp2/junk,lata,lona,timef,presf,rhf,typlev

C                                                                       R6200860
      DATA MON/0,31,59,90,120,151,181,212,243,273,304,334/              R6200870
      DATA LMON/0,31,60,91,121,152,182,213,244,274,305,335/             R6200880
      DATA NCDCMON1/'JAN','FEB','MAR','APR','MAY','JUN',
     > 'JUL','AUG','SEP','OCT','NOV','DEC'/
      DATA NCDCMON2/1,2,3,4,5,6,7,8,9,10,11,12/
      data xws1/mxlev*0.0/,igrd/9999/
      data ccomma/','/
      data tmin/175./,tmax/322./
      data pmin/0.0/,pmax/1040./

c --- Report progress
      write(iomesg,*)'COMPUTATIONAL PHASE'

c --- Pass the starting/ending Julian days to local variables
      ibday=ibjul
      ieday=iejul
c
c --- Write the temperature values used in the QA range checks
      write(io6,6044)tmin,tmax
6044  format(/1x,'Temperature values used in range checks: '/
     1  5x,'TMIN = ',f5.1/
     2  5x,'TMAX = ',f5.1)
c
c --- Write the pressure values used in the QA range checks
      write(io6,6046)pmin,pmax
6046  format(/1x,'Pressure values used in range checks: '/
     1  5x,'PMIN = ',f6.1/
     2  5x,'PMAX = ',f6.1)

c --- Write the heading for the list file table
      WRITE(io6,6050)
6050  FORMAT(//1x,'THE FOLLOWING SOUNDINGS HAVE BEEN PROCESSED:'//1x,
     1     6X,'YEAR',3X,'MONTH',3X,'DAY',5X,'JUL.DAY',2X,
     2 'TIME(HH SSSS UTC)',2X,'NO. LEVELS EXTRACTED'/)
c     2 'HOUR-ENDING(GMT)',2X,'NO. LEVELS EXTRACTED'/)
C                                                                       R6201130
C       INITIALIZE PREVIOUS GOOD SOUNDING TIME                          R6201140
C                                                                       R6201150
      isavyr=ibyr
      IF(IBHR.EQ.0)GO TO 100                                            R6201160
C-----STARTING HOUR = 12                                                R6201170
      JDAY2=IBDAY                                                       R6201180
      ISAV2=0                                                           R6201190
      GO TO 200                                                         R6201200
100   CONTINUE                                                          R6201210
C-----STARTING HOUR = 00                                                R6201220
      JDAY2=IBDAY-1                                                     R6201230
      ISAV2=12                                                          R6201240
200   CONTINUE                                                          R6201250

      isub0=isub
      if(isub.GT.0) then
c ---    Acquire the next available substitute sounding
         call NEXTSUB(isavyr,jday2,isav2,isub0)
      endif
      if(isub0.EQ.0) isub=0

C                                                                       R6201260
C *********************************************************************
C
C-----WRITE HEADER RECORDS
      call WRHD(ifmt,ibyr,ibday,ibhr,ibsec,ieyr,ieday,iehr,iesec,
     1          pstop,jdat,LHT,LTEMP,LWD,LWS,
     2          ielevm,stnid,cname,clat,clon,ver,level)
C
C *********************************************************************
C

c --- Top of LOOP over soundings
c ---------------------------------------------------------------------
1000  CONTINUE                                                          R6201270
      IF (JDAT.EQ.1) THEN
C                                                                       R6201280
C-----READ TD-6201 SOUNDING FROM UNIT ITD                               R6201290
C                                                                       R6201300
c        READ(io8,6100,END=2000) STNID,LAT,LATA,LON,LONA,YEAR,MONTH,    R6201310
c     1  DAY,HOUR,NUMLEV,(QIND(I),ETIME,PRES(I),HEIGHT(I),              R6201320
c     2  TEMP(I),RH,WD(I),WS(I),TIMEF(I),PRESF(I),HGTF(I),TEMPF(I),     R6201330
c     3  RHF,WINDF(I),TYPLEV,I=1,79)                                    R6201340
c ---   Read the full 4-digit year
c6100  FORMAT(3X,A5,I4,A1,I5,A1,2X,4(I2),I3,
c     1      (79(A1,F4.1,F5.1,F6.0,F4.1,3(I3),7A1)))
c6100    FORMAT(3X,A5,I4,A1,I5,A1,i4,3(I2),I3,
c     1        (79(A1,F4.1,F5.1,F6.0,F4.1,3(I3),7A1)))
c
c ---   Allow variable-length record
        SECOND=0
        READ(io8,6100,END=2000) STNID,LAT,LATA,LON,LONA,YEAR,MONTH,
     1  DAY,HOUR,NUMLEV,(QIND(I),ETIME,PRES(I),HEIGHT(I),
     2  TEMP(I),RH,WD(I),WS(I),TIMEF(I),PRESF(I),HGTF(I),TEMPF(I),
     3  RHF,WINDF(I),TYPLEV,I=1,numlev)
6100    FORMAT(A8,I4,A1,I5,A1,i4,3(I2),I3,
     1        (200(A1,F4.1,F5.1,F6.0,F4.1,3(I3),7A1)))
c
c ---   Make sure year is YYYY (Y2K)
        call YR4(io6,yearr,ierr)
        if(ierr.NE.0) stop 'Halted in MAIN (TD-6201) - see list file'
c
        ALAT = LAT/100 + (LAT-(LAT/100*100))/60.                        R6201360
        ALON = LON/100 + (LON-(LON/100*100))/60.                        R6201370

c ---   Check for array dimensioning problems
        if(numlev.gt.mxlev)then
           write(io6,*)'ERROR -- too many levels in upper air data ',
     1     'for array dimensions'
           write(io6,*)'YEAR: ',year,' MONTH: ',month,' DAY: ',day,
     1     ' HOUR: ',hour,' SECOND: ',second
           write(io6,*)'NUMLEV in file = ',numlev
           write(io6,*)'MXLEV in array = ',mxlev
           stop
        endif
c
c ---   Save original number of levels for output
        mlev=numlev
c
c ---   Use 9999. for missing value indicator for height
        do i=1,numlev
          if(height(i).lt.-99998.or.height(i).gt.99998.)height(i)=9999.
        enddo
c
c ---   Set ground elevation as first sounding level (lowest one
c ---   encountered) that is non-missing
        if(height(1).lt.float(igrd).and.height(1).lt.9998.)
     &     igrd=height(1)
c
c1050  IF(NUMLEV.LE.79) GO TO 1100                                       R6201410
c      READ(io8,6150,END=2000) JUNK                                      R6201420
c6150  FORMAT(A32,79(36X))
c      NUMLEV = MAX(NUMLEV-79,79)                                        R6201430
c      GO TO 1050                                                        R6201440
      ELSEIF (JDAT.EQ.2) THEN
C
C-----READ NCDC CD-ROM Data (FSL format)
C
c ---   (DGS) Set point to read next period if there are no data recs
7099    continue
c ---   CD-ROM FSL Header #1
        SECOND=0
        READ(io8,7100,END=2000) HOUR,DAY,MONTHC,YEAR
c ---   Read full 4-digit year
c7100    FORMAT(7X,2I7,6X,A3,6X,I2)
7100    FORMAT(7X,2I7,6X,A3,4X,I4)
c
c ---   Make sure year is YYYY (Y2K)
        call YR4(io6,year,ierr)
        if(ierr.NE.0) stop 'Halted in MAIN (FSL) - see list file'
c
        DO II=1,12
        IF (NCDCMON1(II).EQ.MONTHC) THEN
          MONTH=NCDCMON2(II)
          GOTO 1051
        ENDIF
        ENDDO
c ---   CD-ROM FSL Header #2
c ---   CD format is (3i7,2F7.2,2i7), read as (9x,a5,7X,2F7.2,i7) [OLD]
c ---   WEB format is now (3i7,F7.2,a1,f6.2,a1,i6,i7), where the new A1
c ---   variables are N,S,E, or W for the lat/lon
c ---   Use the new format for both, but do not interpret the N,S,E,W
c ---   (Take STNID from /STATION/ common)
1051    READ(io8,7120,END=2000) ALAT,LATA,ALON,LONA,igrd
c7120    FORMAT(7x,a7,7X,F7.2,a1,f6.2,a1,i6)
7120    FORMAT(21x,F7.2,a1,f6.2,a1,i6)
c ---   CD-ROM FSL Header #3
        READ(io8,7130,END=2000)nlines
7130    FORMAT(28X,I7)
c
c ---   Number of levels = number of lines of data - 4 ID lines
        numlev=nlines-4
        if(numlev.gt.mxlev)then
           write(io6,*)'ERROR -- too many levels in upper air data ',
     1     'for array dimensions'
           write(io6,*)'YEAR: ',year,' MONTHC: ',monthc,' DAY: ',day,
     1     ' HOUR: ',hour,' SECOND: ',second
           write(io6,*)'NLINES = ',nlines,' NUMLEV = ',numlev
           write(io6,*)'MXLEV = ',mxlev
           stop
        endif
c
c ---   Save original number of levels for output
        mlev=numlev
c
c ---   CD-ROM FSL Header #4
        READ(io8,7140,END=2000) WSUNIT
7140    FORMAT(16X,5x,26X,A2)

c ---   (DGS) Get next sounding now if there are no data records
        if(numlev.EQ.0) goto 7099

Cvrt -- Changed to check if levels are out of order (by pressure). If so
cvrt -- then delete that record and get the next record with lower pressure.
cvrt    READ(io8,7150,END=2000) (PRES(I),HEIGHT(I),TEMP(I),
cvrt >   WD(I),WS1(I),I=1,NUMLEV)

        READ(io8,7150,END=2000) (xPRES(I),xHEIGHT(I),xTEMP(I),
     >   xWD(I),iWS1(I),I=1,NUMLEV)
7150    FORMAT(7X,F7.0,F7.0,F7.1,7X,2I7)
         ic = 1
         pres(1) = xpres(1)
         height(1) = xheight(1)
         temp(1) = xtemp(1)
         wd(1) = xwd(1)
         ws1(1) = iws1(1)
         do 700 i=2,numlev
           if (xpres(i).lt.pres(ic)) then
               ic = ic + 1
               pres(ic) = xpres(i)
               height(ic) = xheight(i)
               temp(ic) = xtemp(i)
               wd(ic) = xwd(i)
               ws1(ic) = iws1(i)
           end if
700      continue
         numlev = ic

        DO II=1,NUMLEV
           IF (HEIGHT(II).GT.32766.) HEIGHT(II)=9999.

cvrt missing temps should be 999.9 not 99.9
cvrt       IF (TEMP(II).EQ.3276.7) TEMP(II)=99.9
           IF (TEMP(II).EQ.3276.7) TEMP(II)=999.9
cvrtend

           IF (WD(II).EQ.32767) WD(II)=999
           IF (WS1(II).EQ.32767)xws1(II)=999.9
        ENDDO
        IF (WSUNIT.EQ.'kt') THEN
         DO II=1,NUMLEV
            IF (WS1(II).NE.32767)xws1(II)=WS1(II)*0.514791
         ENDDO
        ELSEIF (WSUNIT.EQ.'ms') THEN
         DO II=1,NUMLEV
            IF (WS1(II).NE.32767)xws1(II)=WS1(II)/10.
         ENDDO
        ENDIF
c
c ---   Compute integer equivalent (WS) of real wind speed (XWS1) -- for
c ---   use in missing value checks
        do ii=1,numlev
           ws(ii)=xws1(ii)
        enddo
      ENDIF
C                                                                       R6201380
C     IF CONTINUATION OF LAST SOUNDING, IGNORE AND READ NEXT SOUNDING   R6201390
C                                                                       R6201400
1100  CONTINUE                                                          R6201450
C                                                                       R6201460
C*******ROUTINE TO CONVERT DATES TO JULIAN                              R6201470
C                                                                       R6201480
        JDAY=MON(MONTH)+DAY                                             R6201490
        IF(MOD(YEAR,4).EQ.0)JDAY=LMON(MONTH)+DAY                        R6201500
C                                                                       R6201530
C       CHECK FOR BEGINNING AND ENDING TIMES                            R6201540
C                                                                       R6201550
      IF(YEAR.LT.IBYR) GO TO 1000                                       R6201560
      IF(YEAR.GT.IEYR) GO TO 5000                                       R6201570
      IF(YEAR.EQ.IBYR.AND.JDAY.LT.IBDAY) GO TO 1000                     R6201580
      IF(YEAR.EQ.IEYR.AND.JDAY.GT.IEDAY) GO TO 5000                     R6201590
      IF(YEAR.EQ.IBYR.AND.JDAY.EQ.IBDAY.AND.HOUR.LT.IBHR) GO TO 1000    R6201600
      IF(YEAR.EQ.IEYR.AND.JDAY.EQ.IEDAY.AND.HOUR.GT.IEHR) GO TO 5000    R6201610

c --- 10/13/2006
c --- Place sounding time as a "snapshot" rather than as a 1-hour
c --- average.  Use the sounding time as both the start and end time.
c
c --- Remove code for 1-hour average treatment...
c
cc --- Convert to explicit beginning ending times with seconds 
cc --- (for output purposes only at this point (FRR 041123)
c      ibyrn=year
c      ibjuln=jday
c      ibhrn=hour
c      ibsecn=0
c      call incr(io6,ibyrn,ibjuln,ibhrn,-1)
c      call grday(io6,ibyrn,ibjuln,ibmon,ibdyn)
c
c      ieyrn=year
c      iejuln=jday
c      iehrn=hour
c      iesecn=0
cc --- For ending time, use midnight=24h on day n, rather than 0h on day n+1
c      if (iehrn.eq.0) then
c         call incr(io6,ieyrn,iejuln,iehrn,-1)
c         iehrn=iehrn+1
c      endif
c      call grday(io6,ieyrn,iejuln,iemon,iedyn)
c
c --- Add code for snapshot treatment...
c
      ibyrn=year
      ibjuln=jday
      ibhrn=hour
      ibsecn=0
      call GRDAY(io6,ibyrn,ibjuln,ibmon,ibdyn)
      ieyrn=ibyrn
      iejuln=ibjuln
      iemon=ibmon
      iedyn=ibdyn
      iehrn=ibhrn
      iesecn=0
c --- 10/13/2006

C                                                                       R6201620
C       COMPRESS ARRAYS IF MISSING VALUES ARE FOUND                     R6201630
C                                                                       R6201640
      KK=0                                                              R6201650
      DO 1200 JJ=1,NUMLEV                                               R6201660
      IF (JDAT.EQ.1) THEN
        IF(GOOD(QIND(JJ)).EQ.0) GO TO 1200                              
        IF(LHT .AND. (HEIGHT(JJ).gt.9998. .OR. GOOD(HGTF).EQ.0))
     1   GO TO 1200                                                     
        IF(LTEMP .AND. (ABS(TEMP(JJ)).GE.99. .OR. GOOD(TEMPF).EQ.0))
     1   GO TO 1200                                                    
        IF(LWD .AND. (WD(JJ).GE.999 .OR. GOOD(WINDF).EQ.0))GO TO 1200
        IF(LWS .AND. (WS(JJ).ge.999 .OR. GOOD(WINDF).EQ.0))GO TO 1200
      ELSEIF (JDAT.EQ.2) THEN
        IF(LHT .AND. HEIGHT(JJ).gt.9998) GO TO 1200                    
        IF(LTEMP .AND. TEMP(JJ).gt.998.) GO TO 1200
        IF(LWD .AND. WD(JJ).ge.999) GO TO 1200
        IF(LWS .AND. WS(JJ).ge.999) GO TO 1200
      ENDIF
      KK=KK+1                                                           R6201740
      APRES(KK)=PRES(JJ)                                                R6201750
      ATEMP(KK)=TEMP(JJ)                                                R6201760
      AWS(KK)=WS(JJ)                                                    R6201770
      xaws1(kk)=xws1(jj)
      AWD(KK)=WD(JJ)                                                    R6201780
      HIGHT(KK)=HEIGHT(JJ)                                              R6201790
1200  CONTINUE                                                          R6201800

      if (kk.eq.0) goto 1000

      NLEV=KK                                                           R6201810
      DO 1300 LL=1,NLEV                                                 R6201820
      PRES(LL)=APRES(LL)                                                R6201830
      TEMP(LL)=ATEMP(LL)                                                R6201840
      WD(LL)=AWD(LL)                                                    R6201850
      WS(LL)=AWS(LL)                                                    R6201860
      xws1(LL)=xaws1(LL)
      HEIGHT(LL)=HIGHT(LL)                                              R6201870
1300  CONTINUE                                                          R6201880
C                                                                       R6201890
C-----DETERMINE LEVELS UP TO PSTOP                                      R6201900
C                                                                       R6201910
      KSTOP = 0                                                         R6201920
      DO 1500 I = 1,NLEV                                                R6201930
      IF(PRES(I).LE.PSTOP) THEN                                         R6201940
          ISTOP = I                                                     R6201950
          GO TO 1600                                                    R6201960
      ENDIF                                                             R6201970
1500  CONTINUE                                                          R6201980
      ISTOP = NLEV                                                      R6201990
      IF(ABS(PRES(NLEV)-PSTOP).GT.1.0) KSTOP = 1                        R6202000
1600  CONTINUE                                                          R6202010

      if (istop.eq.1) goto 1000

c --- SUBSTITUTION for missing soundings
1610  isub0=isub
      more=0
      if(isub.GT.0) then
         call SUBMISS(isavyr,jday2,isav2,year,jday,hour,more,stnid)
         if(more.GT.0) call NEXTSUB(isavyr,jday2,isav2,isub0)
      endif
      isub=isub0
      if(more.GT.0) goto 1610

C                                                                       R6202140
C-----CHECK FOR MISSING DAYS                                            R6202150
C                                                                       R6202160
      IF(JDAY.EQ.JDAY2) THEN
         JDAY1=JDAY
         GO TO 1700
      ELSE                                                              R6202170
         JDAY1=JDAY2
         JDAY2=JDAY
      ENDIF
c*-*-* EMI Modification (7/29/97)
c-emi Allow data to span from one year to the next without a missing
c     day message unless there really is a missing day.
c-emi IF(JDAY1.EQ.(JDAY2-1)) GO TO 1700                                 R6202200
      if(isavyr.EQ.year .and. JDAY1.EQ.(JDAY2-1))then
        go to 1700
      else
        yearm1 = year-1
        if(MOD(isavyr,4).EQ.0)then
          idaysv = 366
        else
          idaysv = 365
        endif
        if(isavyr.EQ.yearm1 .AND. jday1.EQ.idaysv .AND. jday.EQ.1)
     +    go to 1700
      endif

      WRITE(io6,6070)
      WRITE(io9,6070)
6070  FORMAT(1X,'->->->Missing day(s)')
1700  CONTINUE                                                          R6202230
c*-*-* EMI Modification (7/29/97)
      isavyr = year
c*-*-* End of EMI Modification (7/29/97)
C                                                                       R6202240
C-----CHECK FOR MISSING/DUPLICATE SOUNDINGS                             R6202250
C                                                                       R6202260
      ISAV1=ISAV2                                                       R6202270
      ISAV2=HOUR                                                        R6202280

      call deltt(isavyr,jday1,isav1,year,jday,hour,idelhr)
      if(idelhr.NE.0 .AND. idelhr.LE.12) go to 1900

c --- When substituting soundings simply drop duplicate soundings
      if(isub.GT.0 .AND. idelhr.EQ.0) goto 1000

      WRITE(io6,6080)
      WRITE(io9,6080)
6080  FORMAT(1X,'->->->Missing/duplicate sounding or time > 12 hours')
      GO TO 1900                                                        R6202330
1900  CONTINUE                                                          R6202380


c ---------------------------------------------------------------------
c --- QA and Repair Sounding
c ---------------------------------------------------------------------

c --- Initialize QA logicals
      lrmb=.FALSE.
      lrbot=.FALSE.
      lrtop=.FALSE.

      lqamb=.FALSE.
      lqabot=.FALSE.
      lqatop=.FALSE.
      lqafirst=.FALSE.
      lqapht=.FALSE.
      lqazht=.FALSE.
      lqaelev=.FALSE.
      lqawd=.FALSE.
      lqaws=.FALSE.
      lqat=.FALSE.
      lqap=.FALSE.

c --- Change missing value indicator for temperature to 999.9
c --- when using TD-6201 data (JDAT=1).  Already converted to 999.9
c --- when JDAT=2
      if(jdat.eq.1)then
         do LL=1,nlev
c ---       TD-6201 format uses -99.9 as missing temperature flag
            if(temp(LL).le.-99.)temp(LL)=999.9
         enddo
      endif

c --- Convert to Kelvin if temp. is not missing (avoid real equivalence
c --- check)
      do i=1,istop
        if (temp(i).lt.999.) temp(i) = temp(i)+273.2
      enddo

c --- Fill the real WS array if using TD-6201 data (JDAT=1) with a
c --- comma-delimited file
      if(jdat.eq.1.and.ifmt.eq.2)then
         do i=1,istop
            xws1(i)=ws(i)
            if(ws(i).eq.999)xws1(i)=999.9
         enddo
      endif      

c --- Check for non-repairable problems
c -------------------------------------

c --- Check that first level is at the ground
      iel=height(1)
      if(iel.gt.igrd) lqafirst=.TRUE.

c --- Check that the pressure is decreasing with height
      do i=2,istop
         if(pres(i).ge.pres(i-1))lqapht=.TRUE.
      enddo

c --- Check that the elevation is increasing with height
      do i=2,istop
         im1=i-1
         if(height(i).le.height(im1).and.
     1   height(i).le.9998..and.height(im1).le.9998.) lqazht=.TRUE.
      enddo

c --- Check for missing height values
      do i=1,istop
        if(height(i).gt.9998.) lqaelev=.TRUE.
      enddo

c --- Check range of non-missing wind directions
      do i=1,istop
        if(wd(i).lt.999.and.(wd(i).lt.0.or.wd(i).gt.360)) lqawd=.TRUE.
      enddo

c --- Check range of non-missing wind speeds
      do i=1,istop
        if(xws1(i).lt.0.0) lqaws=.TRUE.
      enddo

c --- Check range of non-missing temperatures
      do i=1,istop
        if(temp(i).lt.999.8.and.(temp(i).lt.tmin.or.temp(i).gt.tmax))
     1    lqat=.TRUE.
      enddo

c --- Check range of non-missing pressures (missing value = -99.9)
      do i=1,istop
        if(pres(i).lt.pmin.or.pres(i).gt.pmax) lqap=.TRUE.
      enddo

c --- Check for 'repairable' problems
c -----------------------------------

c --- Check for short sounding
      if(kstop.NE.0) lqamb=.TRUE.

c --- Check for missing data at top of sounding
      if(height(istop).gt.9998..or.temp(istop).gt.999.8.or.
     1 wd(istop).ge.999.or.xws1(istop).gt.998.) lqatop=.TRUE.

c --- Check for missing data at bottom of sounding
      if(height(1).gt.9998..or.temp(1).gt.999.8.or.
     1 wd(1).ge.999.or.xws1(1).gt.998.) lqabot=.TRUE.


c --- Skip any repair attempts if other problems exist
      lrepair=.TRUE.
      if(lqafirst .OR.  lqapht .OR. lqazht .OR. lqaelev .OR.
     &   lqawd .OR. lqaws .OR. lqat .OR. lqap) lrepair=.FALSE.

c --- Repair QA problems at top of sounding if possible
c -----------------------------------------------------
      if(lrepair .AND. lxtop .AND. (lqamb.OR.lqatop)) then
c ---    Extrapolate upwards
         ntop=istop
         if(lqamb) ntop=istop+1
         if(ntop.LE.mxlev) then
c ---       Get valid data levels for extrapolation
            ilevs=1
            ilevd=1
            ilevt1=1
            ilevt2=1
            ilevz1=1
            ilevz2=1
            do i=1,istop
               if(xws1(i).LE.998.) ilevs=i
               if(wd(i).LE.998) ilevd=i
               if(height(i).LE.9998. .AND. pres(i).GT.-98.) ilevz2=i
               if(temp(i).LE.999.8 .AND. height(i).LE.9998.) ilevt2=i
            enddo
            do i=1,(ilevz2-1)
               if(height(i).LE.9998. .AND. pres(i).GT.-98.) ilevz1=i
            enddo
            do i=1,(ilevt2-1)
               if(temp(i).LE.999.8 .AND. height(i).LE.9998.) ilevt1=i
            enddo
c ---       Check against user's constraint
            ii=ilevs
            ii=MIN(ii,ilevd)
            ii=MIN(ii,ilevz2)
            ii=MIN(ii,ilevt2)
            if(pvtop.GE.pres(ii)) then
               if(lqamb) then
c ---             Fill everything at PSTOP level
                  pres(ntop)=pstop
                  wd(ntop)=wd(ilevd)
                  xws1(ntop)=xws1(ilevs)
                  ws(ntop)=ws(ilevs)
                  zslope=(height(ilevz2)-height(ilevz1))/
     &                   (pres(ilevz2)-pres(ilevz1))
                  height(ntop)=height(ilevz2)+zslope*
     &                        (pres(ntop)-pres(ilevz2))
                  tslope=(temp(ilevt2)-temp(ilevt1))/
     &                   (height(ilevt2)-height(ilevt1))
                  temp(ntop)=temp(ilevt2)+tslope*
     &                       (height(ntop)-height(ilevt2))
                  lrmb=.TRUE.
                  istop=ntop
               elseif(lqatop) then
c ---             Fill missing variables at top level
                  if(wd(istop).ge.999) wd(istop)=wd(ilevd)
                  if(xws1(istop).gt.998.) then
                     xws1(istop)=xws1(ilevs)
                     ws(istop)=ws(ilevs)
                  endif
                  if(height(istop).gt.9998.) then
                     zslope=(height(ilevz2)-height(ilevz1))/
     &                      (pres(ilevz2)-pres(ilevz1))
                     height(istop)=height(ilevz2)+zslope*
     &                             (pres(istop)-pres(ilevz2))
                  endif
                  if(temp(istop).gt.999.8) then
                     tslope=(temp(ilevt2)-temp(ilevt1))/
     &                      (height(ilevt2)-height(ilevt1))
                     temp(istop)=temp(ilevt2)+tslope*
     &                           (height(istop)-height(ilevt2))
                  endif
                  lrtop=.TRUE.
               endif
c ---          Reset QA logicals
               lqamb=.FALSE.
               lqatop=.FALSE.
            endif
         endif
      endif

c --- Repair QA problems at bottom of sounding if possible
c --------------------------------------------------------
      if(lrepair .AND. lxsfc .AND. lqabot) then
c ---    Temperature and height cannot be missing
         if(temp(1).LT.999.8 .AND. height(1).LT.9998.) then
c ---       Extrapolate wind to surface               
c ---       Get valid data levels for extrapolation
            ilevs=istop
            ilevd=istop
            do i=istop,1,-1
               if(xws1(i).LE.998. .AND. height(i).LE.9998.) ilevs=i
               if(wd(i).LE.998 .AND. height(i).LE.9998.) ilevd=i
            enddo
c ---       Check against user's constraint
            ii=ilevs
            ii=MAX(i,ilevd)
            zagl=height(ii)-height(1)
            if(zvsfc.GE.zagl) then
c ---          Fill missing variables at surface
               if(wd(1).ge.999) wd(1)=wd(ilevd)
               if(xws1(1).gt.998.) then
                  zagl=height(ilevs)-height(1)
                  if(xws1(ilevs).LE.1.1) then
                     xws1(1)=xws1(ilevs)
                     ws(1)=ws(ilevs)
                  elseif(zagl.LE.11.) then
                     xws1(1)=xws1(ilevs)
                     ws(1)=ws(ilevs)
                  else
                     xws1(1)=xws1(ilevs)*(0.1*zagl)**(-0.15)
                     ws(1)=NINT(xws1(1))
                  endif
               endif
               lrbot=.TRUE.
c ---          Reset QA logical
               lqabot=.FALSE.
            endif
         endif
      endif
c ---------------------------------------------------------------------

c --- Try to replace this sounding with a substitute if any problems
c --- remain
1660  isub0=isub
      more=0
      ireplace=0
      if(isub.GT.0 .AND. (lqamb .OR. lqabot .OR. lqatop .OR. lqafirst
     &                    .OR.  lqapht .OR. lqazht .OR. lqaelev .OR.
     &                    lqawd .OR. lqaws .OR. lqat .OR. lqap)) then
         call SUBREP(isavyr,jday2,isav2,more,ireplace,stnid)
         if(more.GT.0) call NEXTSUB(isavyr,jday2,isav2,isub0)
      endif
      isub=isub0
      if(more.GT.0 .AND. ireplace.EQ.0) then
c ---    Try to replace sounding again
         goto 1660
      elseif(ireplace.EQ.1) then
c ---    Sounding has been replaced, so go on to next period
         goto 1000
      endif

C                                                                       R6202390
C-----WRITE TO LINE PRINTER AND UPPER AIR OUTPUT FILE                   R6202030
C                                                                       R6202040
      WRITE(io6,6060)YEAR,MONTH,DAY,JDAY,HOUR,SECOND,ISTOP
6060   FORMAT(7X,I4,4X,I2,6X,I2,7X,I3,9X,I2,I5,9X,I5)
c --- Explicit beg/ending times with seconds (format 2.1) (FRR 041123)
      WRITE(io9,6200) STNID,IBYRN,IBMON,IBDYN,IBHRN,IBSECN,
     :        IEYRN,IEMON,IEDYN,IEHRN,IESECN, mlev,ISTOP
6200  FORMAT(3X,'6201',2X,A8,2(4x,i4,i4,i3,i3,i5),i5,3x,I5)

      if(lrmb.OR.lrtop) then
         write(io6,*)' ->->->Data at top of sounding repaired'
      endif
      if(lrbot) then
         write(io6,*)' ->->->Data at bottom of sounding repaired'
      endif

      if(lqamb) then
          WRITE(io6,6065) PSTOP
          WRITE(io9,6065) PSTOP
6065      FORMAT(1X,'->->->Top ','of sounding is below the ',F6.1,
     1              '-mb level')
      endif
      if(lqabot) then
         write(io6,*)' ->->->Data at bottom of sounding is missing'
         write(io9,*)' ->->->Data at bottom of sounding is missing'
      endif
      if(lqatop) then
         write(io6,*)' ->->->Data at top of sounding is missing'
         write(io9,*)' ->->->Data at top of sounding is missing'
      endif
      if(lqafirst)then
         write(io6,*)' ->->->First level in sounding is above surface'
         write(io9,*)' ->->->First level in sounding is above surface'
      endif
      if(lqapht)then
         write(io6,*)' ->->->Pressure increasing with height'
         write(io9,*)' ->->->Pressure increasing with height'
      endif
      if(lqazht)then
         write(io6,*)' ->->->Elevation is decreasing with height'
         write(io9,*)' ->->->Elevation is decreasing with height'
      endif      
      if(lqaelev)then
         write(io6,*)' ->->->Elevation missing'
         write(io9,*)' ->->->Elevation missing'
      endif  
      if(lqawd)then
         write(io6,*)' ->->->Invalid wind direction (< 0 or > 360)'
         write(io9,*)' ->->->Invalid wind direction (< 0 or > 360)'
      endif  
      if(lqaws)then
         write(io6,*)' ->->->Invalid wind speed (< 0)'
         write(io9,*)' ->->->Invalid wind speed (< 0)'
      endif   
      if(lqat)then
         write(io6,*)' ->->->Invalid temperature (< TMIN or > TMAX)'
         write(io9,*)' ->->->Invalid temperature (< TMIN or > TMAX)'
      endif
      if(lqap)then
         write(io6,*)' ->->->Invalid or missing pressure ',
     1    '(< PMIN or > PMAX)'
         write(io9,*)' ->->->Invalid or missing pressure ',
     1    '(< PMIN or > PMAX)'
      endif                                  

c -------------------------------------
c --- Write the data to the UP.DAT file
c -------------------------------------
      if(ifmt.EQ.1)then
c ---   Write a slash-delimited file (original format)
        WRITE(io9,6210) (PRES(I),HEIGHT(I),TEMP(I),WD(I),WS(I),
     1   I=1,ISTOP)
6210    FORMAT(4(3X,F6.1,'/',F5.0,'/',F5.1,'/',I3,'/',I3))
      else
c ---   Write a comma-delimited file
        istopm1=istop-1
        WRITE(io9,6211) (PRES(I),HEIGHT(I),TEMP(I),WD(I),xws1(I),
     1   ccomma,I=1,istopm1),
     2   PRES(istop),HEIGHT(istop),TEMP(istop),WD(istop),xws1(istop)
6211    FORMAT(4(3X,F6.1,',',F5.0,',',F5.1,',',I3,',',f5.1,a1))
      endif
C                                                                       R6202140
      GO TO 1000                                                        R6202400
2000  WRITE(io6,6090)IBYRN,IBJULN
6090  FORMAT(/20X,'EOF ON INPUT'/20X,'LAST YR,DAY READ =  ',I4,',',I3)
C                                                                       R6202420
5000  CONTINUE                                                          R6202430

      return                                                            R6202440
      END                                                               R6202770
c-----------------------------------------------------------------------
      INTEGER FUNCTION GOOD(IQUAL)
c-----------------------------------------------------------------------
C                                                                       GOO00030
c --- READ62   Version: 5.661    Level: 941215                    GOOD
C
C PURPOSE: CHECKS QUALITY INDICATOR TO DETERMINE WHETHER OR NOT TO      GOO00040
C           ACCEPT THE UPPER AIR OBSERVATION AS VALID                   GOO00050
C                                                                       GOO00060
C ASSUMPTIONS: TDF6201 FORMAT                                           GOO00070
C                                                                       GOO00080
C LIMITATIONS: NMC INDICATORS A-Z ARE NOT TESTED FOR BAD DATA           GOO00090
C                                                                       GOO00100
C ARGUMENTS                                                             GOO00110
C  PASSED:                                                              GOO00120
C       IQUAL   CHAR    QUALITY INDICATOR: 0-9 OR A-Z                   GOO00130
C                                                                       GOO00140
C  RETURNED:                                                            GOO00150
C       FUNCTION GOOD: 0 IF BAD DATA; 1 IF GOOD DATA                    GOO00160
C                                                                       GOO00170
C  MEANING OF QUALITY INDICATORS FOR TDF6201 DATA:                      GOO00180
C                                                                       GOO00190
C      0       ORIGINAL VALUES ARE CORRECT                              GOO00200
C      1       ORIGINAL VALUES ARE MISSING                              GOO00210
C      2       ORIGINAL VALUES ARE DOUBTFUL, A CORRECTED LEVEL FOLLOWS  GOO00220
C      3       ORIGINAL VALUES ARE DOUBTFUL, UNCORRECTED                GOO00230
C      4       ORIGINAL VALUES ARE IN ERROR, A CORRECTED LEVEL FOLLOWS  GOO00240
C      5       ORIGINAL VALUES ARE IN ERROR, UNCORRECTED                GOO00250
C      6       CORRRECTED LEVEL                                         GOO00260
C      9       LEVEL NOT CHECKED                                        GOO00270
C     A-Z      SUPPLIED BY NMC, HAVE CHANGED MANY TIMES OVER THE YEARS  GOO00280
C                                                                       GOO00290
C       GOOD RETURNS 0 IF CODE IS 1, 2, 3, 4, OR 5; 1 OTHERWISE         GOO00300
C                                                                       GOO00310
C                                                                       GOO00320
C CALLING ROUTINES: READ62                                              GOO00330
C                                                                       GOO00340
C-----------------------------------------------------------------------GOO00350
C                                                                       GOO00360
        CHARACTER*1 IQUAL                                               GOO00380
C                                                                       GOO00390
        GOOD = 1                                                        GOO00400
        IF(IQUAL.EQ.'1' .OR. IQUAL.EQ.'2' .OR. IQUAL.EQ.'3' .OR.        GOO00410
     1   IQUAL.EQ.'4' .OR. IQUAL.EQ.'5') GOOD = 0                       GOO00420
        RETURN                                                          GOO00430
        END                                                             GOO00440
c----------------------------------------------------------------------
      subroutine wrhd(iform,ibyr,ibjul,ibhr,ibsec,
     1                ieyr,iejul,iehr,iesec,
     2                pstop,jdat,LHT,LTEMP,LWD,LWS,
     3                ielevm,stnid,cname,clat,clon,cver,clev)
c----------------------------------------------------------------------
c
c --- READ62     Version: 5.661   Level: 061020                   WRHD
c ---            J. Scire, D. Strimaitis, TRC Environmental Corp.
c
c --- Write the header records for the UP.DAT file
c
c --- UPDATES:
c --- V 5.63 Level 061020  ====> V 5.65 Level 080919 (DGS)
c           - Move the time zone record so that it appears just
c             before the time information record when station
c             location information is written
c
c --- V 5.6 Level 041123  ====> V 5.63 Level 061020 (DGS)
c           - Change output begin time to be the time provided in 
c             the control file (no end-hour shift), and explicitly
c             pass the seconds (UP.DAT Dataset version 2.2)
c
c --- V 5.5 Level 030402  ====> V 5.6 Level 041123 (F.Robe)
c           - Write explicit beginning/ending times with seconds 
c             (not hour-ending convention). New UP.DAT format 2.1
c           - Write out time zone in header (time zone is O as
c             UTC/GMT time is used in UP.DAT) - Extra line -
c      
c --- V 5.2 Level 020805  ====> V 5.5 Level 030402 (D. Strimaitis)
c           - New header record structure
c
c --- V 5.1 Level 020614  ====> V 5.2 Level 020805 (D. Strimaitis)
c           - Toggle header format using ENVPOP parameter
c           - Include PARAMS.R62 and remove IO unit from arg list
c
c --- V 5.1 Level 020330  ====> V 5.1 Level 020614 (D. Strimaitis)
c           - Station ID as character variable
c
c
c --- INPUTS:
c             IFORM - integer    - Data format flag (1=slash delimiter,
c                                  2=comma delimiter - more precision)
c              IBYR - integer    - Beginning year of data (4 digits)
c             IBJUL - integer    - Beginning Julian day number
c              IBHR - integer    - Beginning time (hour)
c             IBSEC - integer    - Beginning time (second)
c              IEYR - integer    - Ending year of data (4 digits)
c             IEJUL - integer    - Ending Julian day number
c              IEHR - integer    - Ending time (hour)
c             IESEC - integer    - Ending time (second)
c             PSTOP - integer    - Top pressure level (mb) of data
c              JDAT - integer    - Original data format (1=TD-6201,
c                                  2=NCDC FSL)
c               LHT - logical    - Drop Sounding level if no height
c             LTEMP - logical    - Drop Sounding level if no temperature
c               LWD - logical    - Drop Sounding level if no direction
c               LWS - logical    - Drop Sounding level if no speed
c            IELEVM - integer    - station elevation (m MSL)
c          STNID - character*8   - station identification code
c          CNAME - character*4   - Name of each station (----)
c           CLAT - character*16  - Latitude of each station (deg[N/S])
c           CLON - character*16  - Longitude of each station (deg[E/W])
c           CVER - character*12  - Version of processor
c           CLEV - character*12  - Level of processor
c
c --- OUTPUT:  none
c
c --- WRHD called by:  COMP
c --- WRHD calls:      none
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.r62'
c
      logical lht,ltemp,lwd,lws

      character*4 cname
      character*8 stnid
      character*16 clat,clon
      character*12 cver,clev
      character*1 q

      character*4 xyunit
      character*8 datum, pmap
      character*12 daten
      character*16 dataset,dataver
      character*64 datamod
      character*80 comment1

c --- Configure output variables
      data dataset/'UP.DAT'/, dataver/'2.1'/
      data datamod/'Hour Start and End Times with Seconds'/
      data ncomment/1/
      data comment1/'Produced by READ62 Version: '/

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
      if(envpop.EQ.0) then
         pmap='NONE    '
      elseif(envpop.EQ.1) then
         pmap='LL      '
         datum='WGS-G   '
         daten='10-10-2002  '
         xyunit='DEG '
      endif


c --- 10/13/2006
c --- Treat sounding time as a "snapshot"
c --- Remove code for 1-hour average treatment...
c
cc --- Convert single hour-ending time to explicit beg/end times with seconds
cc --- for output (041123)
c      ibyrn=ibyr
c      ibjuln=ibjul
c      ibhrn=ibhr
c      ibsecn=0
c      call incr(io6,ibyrn,ibjuln,ibhrn,-1)
c      ieyrn=ieyr
c      iejuln=iejul
c      iehrn=iehr      
c      iesecn=0
cc --- If ending time, use 24 to refer to midnight, not next day 0h
c      if (iehrn.eq.0) then
c         call incr(io6,ieyrn,iejuln,iehrn,-1)
c         iehrn=iehrn+1
c      endif
c
c --- 10/13/2006


c --- Write header
      write(io9,'(2a16,a64)') dataset,dataver,datamod
      write(io9,'(i4)') ncomment
      write(io9,'(a80)') comment1
      write(io9,'(a8)') pmap

      if(envpop.EQ.0) then
c ---    Header without location data
         write(io9,'(a8)')'UTC+0000'
         WRITE (io9,'(1X,8I5,F5.0,2I5)')IBYR,IBJUL,IBHR,IBSEC,
     &                     IEYR,IEJUL,IEHR,IESEC,PSTOP,JDAT,iform
         WRITE (io9,'(1X,4(4X,L1))')LHT,LTEMP,LWD,LWS
      elseif(envpop.EQ.1) then
c ---    Header with location data
         write(io9,'(a8,a12)') datum,daten
         write(io9,'(a4)') xyunit
         write(io9,'(a8)')'UTC+0000'
         WRITE (io9,'(1X,8I5,F5.0,2I5)')IBYR,IBJUL,IBHR,IBSEC,
     &                     IEYR,IEJUL,IEHR,IESEC,PSTOP,JDAT,iform
         WRITE (io9,'(1X,4(4X,L1))')LHT,LTEMP,LWD,LWS
         write(io9,20)stnid,q,cname,q,q,clat,q,q,clon,q,ielevm
      else
         write(*,*)'WRHD:  invalid parameter ENVPOP in PARAMS.R62'
         write(*,*)'       Expected 0 or 1'
         write(*,*)'       Found ',envpop
         stop
      endif

20    format(a8,2x,a1,a4,a1,2x,2(a1,a16,a1,2x),i7)

      return
      end
c----------------------------------------------------------------------
      subroutine rdhdsub(io,dataversub)
c----------------------------------------------------------------------
c
c --- READ62     Version: 5.661   Level: 080919               RDHDSUB
c ---            D. Strimaitis, TRC Environmental Corp.
c
c --- Skip the header records for the UP.DAT file (for substitutions)
c
c --- UPDATES:
c --- V 5.6 Level 041123 ====> V 5.65 Level 080919 (D. Strimaitis)
c           - Place time zone reads just before time record for
c             sub-UP.DAT version 2.1 and 2.2
c
c --- V 5.5 Level 030402 ====> V 5.6 Level 041123 (F.Robe)
c           - Pass sub.data version number through calling list
c           - Skip extra record (time zone) if sub-UP.DAT version 2.1
c
c --- V 5.4 Level 020330  ====> V 5.5 Level 030402 (D. Strimaitis)
c           - New header record structure
c
c --- INPUTS:
c                IO - integer    - Fortran unit no. of data file
c
c --- OUTPUT:  none
c        DATAVERSUB - charc*16   - Substitute UP.DAT version number
c
c --- RDHDSUB called by:  SETUP
c --- RDHDSUB calls:      none
c----------------------------------------------------------------------
c
      character*8 pmap,axtz
      character*16 dataset,dataversub
      character*64 datamod
      character*80 comment1

      
      read(io,'(2a16,a64)') dataset,dataversub,datamod
c --- Check first field of record 1
      if(dataset.EQ.'UP.DAT') then
         read(io,*) ncomment
         do i=1,ncomment
            read(io,'(a80)') comment1
         enddo
         read(io,'(a8)') pmap
         if(pmap.EQ.'NONE   ') then
c ---       UP.DAT time zone must be UTC+0000 (GMT time always)
            if (dataversub.eq.'2.1' .OR. dataversub.eq.'2.2') then
               read(io,'(a8)')axtz
               read(axtz(5:8),'(i4)')itz
               if(itz.ne.0)
     &         stop 'Time zone of substitute UP.DAT must be UTC+0000'
            endif
c ---       Skip remaining 2 header records
            read(io,*)
            read(io,*)
         else
c ---       Skip next 2 header records
            read(io,*)
            read(io,*)
c ---       UP.DAT time zone must be UTC+0000 (GMT time always)
            if (dataversub.eq.'2.1' .OR. dataversub.eq.'2.2') then
               read(io,'(a8)')axtz
               read(axtz(5:8),'(i4)')itz
               if(itz.ne.0)
     &         stop 'Time zone of substitute UP.DAT must be UTC+0000'
            endif
c ---       Skip remaining 3 header records
            read(io,*)
            read(io,*)
            read(io,*)
         endif
      elseif(dataset.EQ.'UPPER-AIR-MET') then
c ---    Skip remaining 3 header records of older header
         read(io,*)
         read(io,*)
         read(io,*)
      else
c ---    Must be oldest header with 2 records
c ---    Skip remaining header record
         read(io,*)
      endif

      return
      end
c----------------------------------------------------------------------
      subroutine nextsub(lastyr,lastjul,lasthr,isub0)
c----------------------------------------------------------------------
c
c --- READ62   Version: 5.661    Level: 020330                 NEXTSUB
c ---          D. Strimaitis,  Earth Tech, Inc.
c
c --- PURPOSE:  Access data in the substitution UP.DAT file
c               to obtain the next available sounding arrays
c
c --- INPUTS:
c               LASTYR - integer    - Year of last sounding (GMT)
c              LASTJUL - integer    - Julian day (GMT) of last sounding
c               LASTHR - integer    - Time (GMT) of last sounding
c
c        Common block /submet/ variables
c            NTZSUB
c
c        Parameters: IO6
c
c --- OUTPUT:
c                ISUB0 - integer    - ISUB (altered if EOF reached)
c
c
c --- NEXTSUB called by:  COMP
c --- NEXTSUB calls:      RDSUB
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.r62'
      include 'control.r62'
      include 'submet.r62'

c --- Set ISUB0
      isub0=isub

c --- Set last date-time
      ilast=lastyr*100000+lastjul*100+lasthr

c --- Get substitute UP.DAT data
10    call RDSUB(ierr)
      if(ierr.NE.0) then
         isub0=0
         write(io6,*)'       No further substitutions are made'
         write(io6,*)'       after YYYYJJJHH = ',ilast
         write(io6,*)
         write(io6,*)
         return
      endif

c --- Check for date-time later than last
      if(ilast.GE.NTZSUB) goto 10

      return
      end
c----------------------------------------------------------------------
      subroutine rdsub(ierr)
c----------------------------------------------------------------------
c
c --- READ62   Version: 5.661    Level: 080919                   RDSUB
c ---          J. Scire, D. Strimaitis
c
c --- PURPOSE:  Read a set of upper air soundings -- input data
c               include:  pressure (mb), height (m above MSL),
c               temp (deg. C), wind direction (deg.), and
c               wind speed (m/s)
c
c --- UPDATES:
c     V5.63 Level 061020  =====> V5.65 Level 080919 (DGS)
c           - Fix wrong format statement line number for time record
c     V5.6 Level 041123  =====> V5.63 Level 061020 (DGS)
c           - Use format (2.1) end-time directly (do not recompute
c             from begin-time), and report this end-time as the
c             sounding time
c     V5.4. Level 020330 =====> V5.6 Level 041123 (041123) (F.Robe)
c           - Read new upper air format (2.1) with beginning
c             and ending times (dates,hours/seconds)
c
c --- INPUTS:
c
c        Common block /CONTROL/ variables
c            ISUB, ITIMESEC
c        Parameters: MXLEV, IO6, IO18
c
c --- OUTPUT:
c                 IERR - integer       - Error flag for read access
c                                        0 = No Error
c                                        1 = Error
c
c        Common block /submet/ variables
c            PSUB(mxlev), ZLSUB(mxlev), TZSUB(mxlev),
c            IWDSUB(mxlev), WSSUB(mxlev), IWSSUB(mxlev),
c            NLSUB,NLORI,NLSUB,NTZSUB,iyrsub,imosub,idysub,julsub,
c            ihrsub
c
c
c --- RDSUB called by:  NEXTSUB
c --- RDSUB calls:      JULDAY, YR4
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.r62'
      include 'control.r62'
      include 'submet.r62'

      ierr=0

c --- Read data header

      if (dataversub.eq.'2.1') then
c ---    explicit beginning/ending time format (2.1)
         read(io18,31,end=999) ibyrn,ibmon,ibdyn,ibhrn,ibsecn,
     :     ieyrn,iemon,iedyn,iehrn,iesecn,nlori,nlsub
31       format(17x,2(4x,i4,i4,i3,i3,i5),i5,3x,I5)
         call YR4(io6,ibyrn,ierr)
         if(ierr.NE.0) stop 'Y2K problem- Halted in RDSUB'
         call JULDAY(io6,ibyrn,ibmon,ibdyn,ibjuln)
         call YR4(io6,ieyrn,ierr)
         if(ierr.NE.0) stop 'Y2K problem- Halted in RDSUB'
         call JULDAY(io6,ieyrn,iemon,iedyn,iejuln)

         if(ibsecn.GE.3600) then
            nhrinc=ibsecn/3600
            ibsecn=ibsecn-nhrinc*3600
            call INCR(io6,ibyrn,ibjuln,ibhrn,nhrinc)
            call GRDAY(io6,ibyrn,ibjuln,ibmon,ibdyn)
         endif
         if(iesecn.GE.3600) then
            nhrinc=iesecn/3600
            iesecn=iesecn-nhrinc*3600
            call INCR(io6,ieyrn,iejuln,iehrn,nhrinc)
            call GRDAY(io6,ieyrn,iejuln,iemon,iedyn)
         endif
c ---    Use ending time as the sounding time
         iyr=ieyrn
         ijul=iejuln
         imo=iemon
         idy=iedyn
         ihr=iehrn
      else 
c ---    Old format (pre-2.1) with hour-ending times      
         read(io18,3,end=999) iyr,imo,idy,ihr,nlori,nlsub
3        format(20x,i4,3i2,2x,i5,T66,i5)

         call YR4(io6,iyr,ierr)
         if(ierr.NE.0) stop 'Y2K problem- Halted in RDSUB'
         call JULDAY(io6,iyr,imo,idy,ijul)
         
      endif   

c --- Pass date-time to /SUBMET/
      iyrsub=iyr
      imosub=imo
      idysub=idy
      julsub=ijul
      ihrsub=ihr
      ntzsub = iyr*100000 + ijul*100 + ihr

c --- Error checks
      if(nlsub.lt.1)then
         write(io6,81) iyr,imo,idy,ihr
81       format(//1x,'ERROR IN SUBR. RDSUB -- no upper air data ',
     1   'for substitution '//1x,'(year,month,day) = (',
     2   i4,',',i2,',',i2,')',2x,'hour = ',i2,' (GMT)')
         stop
      endif

c --- Check that no. levels does not exceed array dimension
      if(nlsub.gt.mxlev)then
         write(io6,86)iyr,imo,idy,ihr,nlsub,mxlev
86       format(//1x,'ERROR IN SUBR. RDSUB -- too many levels for ',
     1   'substitution array dimension'//1x,
     2   '(year,month,day) = (',i4,',',i2,',',i2,')',2x,'hour = ',i2,
     3   ' GMT'//1x,'No. levels = ',i5,3x,'Current array dimension = ',
     4   i5)
         stop
      endif
c
c --- Read data records
      if(isub.eq.1)then
c ---    Original slash-delimited format
         read(io18,4)(psub(ii),zlsub(ii),tzsub(ii),
     1   iwdsub(ii),iwssub(ii),ii=1,nlsub)
4        format(4(3x,f6.1,1x,f5.0,1x,f5.1,1x,i3,1x,i3))
      elseif(isub.eq.2)then
c ---    Comma-delimited data format
         read(io18,*)(psub(ii),zlsub(ii),tzsub(ii),
     1   iwdsub(ii),wssub(ii),ii=1,nlsub)
      else
         write(io6,*)'ERROR in SUBR. RDSUB - Invalid format type - ',
     1   'ISUB = ',isub
         stop
      endif

c --- Set real/integer versions of speed
      if(isub.EQ.1) then
         do ii=1,nlsub
            wssub(ii)=iwssub(ii)
            if(iwssub(ii).EQ.999) wssub(ii)=999.9
         enddo
      else
         do ii=1,nlsub
            iwssub(ii)=wssub(ii)
         enddo
      endif

      return

999   ierr=1
      write(io6,*)
      write(io6,*)
      write(io6,*)
      write(io6,*)'-------------------------------------------------'
      write(io6,*)'RDSUB: EOF reached in substitute UP.DAT file'
      write(io6,*)'-------------------------------------------------'
      write(io6,*)
      return

      end
c----------------------------------------------------------------------
      subroutine submiss(lastyr,lastjul,lasthr,nowyr,nowjul,nowhr,
     &                   more,stnid)
c----------------------------------------------------------------------
c
c --- READ62   Version: 5.661    Level: 100621                 SUBMISS
c ---          D. Strimaitis
c
c --- PURPOSE:  Use data in the substitution UP.DAT file to fill in
c               missing soundings (does one sounding per call)
c
c --- UPDATES:
c ---  V5.63 Level 061020  ====> V5.66 Level 100621 (DGS)
c           - Use DELTT to assess when to invoke the substitute
c             sounding to avoid direct comparisons of date-time that
c             may not use the same midnight convention
c ---  V5.6 Level 041123  ====> V5.63 Level 061020 (DGS)
c           - Sounding start and end times are equal
c ---  V 5.1 Level 020614  ====> V5.6 Level 041123 (F.Robe)
c           - Write out explicit beginning/ending times in UP.DAT
c             (not hour-ending times)
cc --- V 5.1 Level 020330  ====> V 5.1 Level 020614 (D. Strimaitis)
c           - Allow station ID to be up to 8 characters
c
c --- INPUTS:
c               LASTYR - integer    - Year of last sounding (GMT)
c              LASTJUL - integer    - Julian day (GMT) of last sounding
c               LASTHR - integer    - Time (GMT) of last sounding
c                NOWYR - integer    - Year of current sounding (GMT)
c               NOWJUL - integer    - Julian day (GMT) of current
c                                     sounding
c                NOWHR - integer    - Time (GMT) of current sounding
c                STNID - character*8- Station ID
c
c        Common block /submet/ variables
c            (all)
c
c        Parameters: IO6
c
c --- OUTPUT:
c               LASTYR - integer    - Year of last sounding (GMT)
c              LASTJUL - integer    - Julian day (GMT) of last sounding
c               LASTHR - integer    - Time (GMT) of last sounding
c                 MORE - integer    - Processing flag
c                                       0: processing complete
c                                       1: get next substitute sounding
c                                          and call again
c
c --- SUBMISS called by:  COMP
c --- SUBMISS calls:      DELTT
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.r62'
      include 'control.r62'
      include 'submet.r62'

      character*8 stnid
      character*1 ccomma
      data ccomma/','/

      data isec/0/

c --- Hours from last to current sounding
      call DELTT(lastyr,lastjul,lasthr,nowyr,nowjul,nowhr,
     &           idelhr)

c --- Hours from last sounding to current substitute
      call DELTT(lastyr,lastjul,lasthr,iyrsub,julsub,ihrsub,
     &           idelhrsub_last)

c --- Hours from current sounding to current substitute
      call DELTT(nowyr,nowjul,nowhr,iyrsub,julsub,ihrsub,
     &           idelhrsub_now)

      if(idelhr.LT.0) then
c ---    Sounding times are out of order
         write(io6,*)'ERROR in subr. SUBMISS -- Times are out of order'
         write(io6,*)'  Last Sounding Year, Julian Day, Hour: ',
     &                  lastyr,lastjul,lasthr
         write(io6,*)'  Current Sounding Year, Julian Day, Hour: ',
     &                  nowyr,nowjul,nowhr
         write(*,*) 'Halted in SUBMISS -- See List File'
         stop
      endif

      if(idelhrsub_last.LE.0) then
c ---    Get next substitute sounding when current substitution time is
c ---    older than last good sounding already processed
         more=1
      elseif(idelhrsub_now.GE.0) then
c ---    No substitution is possible when current substitution time is
c ---    newer than current sounding
         more=0
      elseif(idelhrsub_now.LT.0) then
c ---    Substitute sounding is between last and current soundings
         if(idelhrsub_last.LE.6) then
c ---       Substitution time is too near last, so get the next one
            more=1
         else
c ---       Use substitute sounding
c ---       Report message if any soundings are still missing
            if(idelhrsub_last.GT.18) then
               WRITE(io6,6080)
               WRITE(io9,6080)
6080           FORMAT(1X,'->->->Missing sounding(s)')
            endif
c ---       Data header and list file log record
            WRITE(io6,6060)iyrsub,imosub,idysub,julsub,ihrsub,nlsub
            write(io6,*)' ->->->Data SUBSTITUTED from alternate file'
6060        FORMAT(7X,I4,4X,I2,6X,I2,7X,I3,9X,I2,14X,I5)

cc ---       Compute explicit beginning/ending times and write them out(041123)
c            ibyrn=iyrsub
c            ibjuln=julsub
c            ibhrn=ihrsub
c            ibsecn=0
c            call incr(io6,ibyrn,ibjuln,ibhrn,-1)
c            call grday(io6,ibyrn,ibjuln,ibmon,ibdyn)
c
c            ieyrn=iyrsub
c            iejuln= julsub
c            iehrn=ihrsub
c            call incr(io6,ieyrn,iejuln,iehrn,-1)
c            call grday(io6,ieyrn,iejuln,iemon,iedyn)
c            iesecn=3600 
c            WRITE(io9,6200) STNID,IBYRN,IBMON,IBDYN,IBHRN,IBSECN,
c     :        IEYRN,IEMON,IEDYN,IEHRN,IESECN, nlori,nlsub

c ---       Start time = end time
            WRITE(io9,6200) STNID,iyrsub,imosub,idysub,ihrsub,isec,
     &                      iyrsub,imosub,idysub,ihrsub,isec,
     &                      nlori,nlsub
6200        FORMAT(3X,'6201',2X,A8,2(4x,i4,i4,i3,i3,i5),i5,3x,I5)

c            WRITE(io9,6200) STNID,iyrsub,imosub,idysub,ihrsub,nlori,
c     &                      nlsub
c6200        FORMAT(3X,'6201',2X,A8,3X,i4,3I2,2X,I5,T66,I5)


c ---       Data record
            if(ifmt.EQ.1)then
c ---         Write a slash-delimited file (original format)
              WRITE(io9,6210) (psub(I),zlsub(I),tzsub(I),iwdsub(I),
     1                         iwssub(I),I=1,nlsub)
6210          FORMAT(4(3X,F6.1,'/',F5.0,'/',F5.1,'/',I3,'/',I3))
            else
c ---         Write a comma-delimited file
              nlsubm1=nlsub-1
              WRITE(io9,6211) (psub(I),zlsub(I),tzsub(I),iwdsub(I),
     1                         wssub(I),ccomma,I=1,nlsubm1),
     2         psub(nlsub),zlsub(nlsub),tzsub(nlsub),iwdsub(nlsub),
     3         wssub(nlsub)
6211          FORMAT(4(3X,F6.1,',',F5.0,',',F5.1,',',I3,',',f5.1,a1))
            endif
c ---       Update LAST sounding time
            lastyr=iyrsub
            lastjul=julsub
            lasthr=ihrsub
            more=1
         endif
      endif         

      return
      end
c----------------------------------------------------------------------
      subroutine subrep(nowyr,nowjul,nowhr,more,ireplace,stnid)
c----------------------------------------------------------------------
c
c --- READ62   Version: 5.661    Level: 100621                  SUBREP
c ---          D. Strimaitis
c
c --- PURPOSE:  Use data in the substitution UP.DAT file to replace
c               a bad sounding
c
c --- UPDATES:
c ---  V5.63 Level 061020  ====> V5.66 Level 100621 (DGS)
c           - Reduce the time difference between the substitute and
c             observed sounding to 3 hours
c ---  V 5.1 Level 020614  ====> V5.63 Level 061020 (DGS)
c           - Write out explicit beginning/ending times in UP.DAT
c           - Sounding start and end times are equal
c --- V 5.1 Level 020330  ====> V 5.1 Level 020614 (D. Strimaitis)
c           - Allow station ID to be up to 8 characters
c
c --- INPUTS:
c                NOWYR - integer    - Year of current sounding (GMT)
c               NOWJUL - integer    - Julian day (GMT) of current
c                                     sounding
c                NOWHR - integer    - Time (GMT) of current sounding
c                STNID - character*8- Station ID
c
c        Common block /submet/ variables
c            (all)
c
c        Parameters: IO6
c
c --- OUTPUT:
c                NOWYR - integer    - Year of current sounding (GMT)
c               NOWJUL - integer    - Julian day (GMT) of current
c                                     sounding
c                NOWHR - integer    - Time (GMT) of current sounding
c                 MORE - integer    - Next sounding needed flag
c                                       0: retain current sounding
c                                       1: get next substitute sounding
c             IREPLACE - integer    - Processing result
c                                       0: sounding NOT replaced
c                                       1: sounding replaced
c
c --- SUBREP called by:  COMP
c --- SUBREP calls:      DELTT
c----------------------------------------------------------------------
c
c --- Include parameters
      include 'params.r62'
      include 'control.r62'
      include 'submet.r62'

      character*8 stnid
      character*1 ccomma
      data ccomma/','/

      data isec/0/

c --- Check sounding times 
      call DELTT(nowyr,nowjul,nowhr,iyrsub,julsub,ihrsub,idelhr)

      if(idelhr.LT.-3) then
c ---    Get next substitute sounding when current substitution time is
c ---    significantly older than current sounding
         more=1
         ireplace=0
      elseif(idelhr.GT.3) then
c ---    No substitution is possible when current substitution time is
c ---    significantly newer than current sounding
         more=0
         ireplace=0
      else
c ---    Use substitute sounding
c ---    Data header and list file log record
         WRITE(io6,6060)iyrsub,imosub,idysub,julsub,ihrsub,nlsub
         write(io6,*)' ->->->Data SUBSTITUTED from alternate file'
6060     FORMAT(7X,I4,4X,I2,6X,I2,7X,I3,9X,I2,14X,I5)


c         WRITE(io9,6200) STNID,iyrsub,imosub,idysub,ihrsub,nlori,
c     &                   nlsub
c6200     FORMAT(3X,'6201',2X,A8,3X,i4,3I2,2X,I5,T66,I5)

c ---    Explicit start time = end time
         WRITE(io9,6200) STNID,iyrsub,imosub,idysub,ihrsub,isec,
     &                   iyrsub,imosub,idysub,ihrsub,isec,
     &                   nlori,nlsub
6200     FORMAT(3X,'6201',2X,A8,2(4x,i4,i4,i3,i3,i5),i5,3x,I5)


c ---    Data record
         if(ifmt.EQ.1)then
c ---      Write a slash-delimited file (original format)
           WRITE(io9,6210) (psub(I),zlsub(I),tzsub(I),iwdsub(I),
     1                      iwssub(I),I=1,nlsub)
6210       FORMAT(4(3X,F6.1,'/',F5.0,'/',F5.1,'/',I3,'/',I3))
         else
c ---      Write a comma-delimited file
           nlsubm1=nlsub-1
           WRITE(io9,6211) (psub(I),zlsub(I),tzsub(I),iwdsub(I),
     1                      wssub(I),ccomma,I=1,nlsubm1),
     2      psub(nlsub),zlsub(nlsub),tzsub(nlsub),iwdsub(nlsub),
     3      wssub(nlsub)
6211       FORMAT(4(3X,F6.1,',',F5.0,',',F5.1,',',I3,',',f5.1,a1))
         endif
c ---    Update sounding time
         nowyr=iyrsub
         nowjul=julsub
         nowhr=ihrsub
         more=1
         ireplace=1
      endif

      return
      end
c----------------------------------------------------------------------
      subroutine RDSTA
c----------------------------------------------------------------------
c
c --- READ62     Version: 5.661   Level: 080919                  RDSTA
c ---            D. Strimaitis
c
c --- Read station information from the top of the upper-air data file
c
c --- UPDATES:
c --- V 5.5 Level 030402  ====> V 5.65 Level 080919 (D. Strimaitis)
c           - Change null characters to blanks in lat/lon strings
c --- V 5.2 Level 020805  ====> V 5.5 Level 030402 (D. Strimaitis)
c           - Always use WMO ID in FSL file, not the WBAN ID
c           - Fix N,S,E,W logic for WEB FSL format
c           - Add list-file output of Station ID
c --- V 5.1 Level 020614  ====> V 5.2 Level 020805 (D. Strimaitis)
c           - Change FSL header format for new WEB structure
c --- V 5.1 Level 020330  ====> V 5.1 Level 020614 (D. Strimaitis)
c           - Station ID as character variable
c
c --- INPUTS:
c
c     Common block /CONTROL/
c        JDAT
c
c --- Parameters used:
c        IO6, IO8
c
c --- OUTPUT:
c
c     Common block /STATION/
c        ielevm,stnid,cname,clat,clon
c        
c
c --- RDSTA called by:  SETUP
c --- RDSTA calls:      none
c----------------------------------------------------------------------
c
      include 'params.r62'
      include 'control.r62'
      include 'station.r62'

      character*1 cNS,cEW
      character*12 clat12,clon12

      if(jdat.EQ.1) then
c ---    TD-6201 format file
c ---    Read part of first record for info
         read(io8,10) stnid,ilat,cNS,ilon,cEW,ielevm
10       format(a8,i4,a1,i5,a1,23x,i6)
c ---    Convert latitude to degrees
         ideg=ilat/100
         imin=ilat-100*ideg
         xlat=ideg+imin/60.0
c ---    Convert longitude to degrees
         ideg=ilon/100
         imin=ilon-100*ideg
         xlon=ideg+imin/60.0
         write(io6,*)
         write(io6,*)'TD-6201 Station ID:      ',stnid
         write(io6,*)

      elseif(jdat.EQ.2) then
c ---    NCDC FSL format file
c ---    Header #1
         READ(io8,'(i7)') itype
c ---    Header #2
c ---    CD format is (3i7,2F7.2,2i7),
c ---    WEB format is now (3i7,F7.2,a1,f6.2,a1,i6,i7), where the new A1
c ---    variables are N,S,E, or W for the lat/lon
c ---    Use the new format for both, and interpret the N,S,E,W as
c ---    N or W if missing (CD convention is N Lat and W Lon)
         READ(io8,'(3i7,F7.2,a1,f6.2,a1,i6)') itype,iwban,iwmo,
     &                                        xlat,cNS,xlon,cEW,ielevm
c ---    Set lat/lon convention
         if(cNS.EQ.'s') cNS='S'
         if(cNS.NE.'S') cNS='N'
         if(cEW.EQ.'e') cEW='E'
         if(cEW.NE.'E') cEW='W'
c ---    Header #3
         READ(io8,'(i7)') itype
c ---    Header #4
         READ(io8,'(i7,10x,a4)') itype,cname
         if(iwban.NE.99999) then
c ---       Use WBAN ID
            write(stnid,'(i8)') iwban
         else
c ---       Use WMO ID
            write(stnid,'(i8)') iwmo
         endif
         write(io6,*)
         write(io6,*)'FSL Station ID used:      ',stnid
         write(io6,*)'            WBAN ID:      ',iwban
         write(io6,*)'             WMO ID:      ',iwmo
         write(io6,*)

      else
         write(io6,*)
         write(io6,*)'RDSTA:  FATAL ERROR!'
         write(io6,*)' Invalid value of JDAT (Upper-Air File format)'
         write(io6,*)' Expected JDAT = 1 or 2'
         write(io6,*)' Found    JDAT = ',jdat

         write(*,*)
         write(*,*)'RDSTA:  FATAL ERROR!'
         write(*,*)' Invalid value of JDAT (Upper-Air File format)'
         write(*,*)' Expected JDAT = 1 or 2'
         write(*,*)' Found    JDAT = ',jdat

         stop
      endif

c --- Form lat/lon character variables
      clat='                '
      write(clat12,'(f12.7)') xlat
      clat(1:12)=clat12
      clat(14:14)=cNS
      write(clon12,'(f12.7)') xlon
      clon='                '
      clon(1:12)=clon12
      clon(14:14)=cEW

c --- Reset position to start of data file
      REWIND(io8)

      return
      end
c----------------------------------------------------------------------
      subroutine fin
c----------------------------------------------------------------------
c
c --- READ62   Version: 5.661          Level: 090511               FIN
c ---          J. Scire
c
c --- PURPOSE:  Run termination routine -- compute runtime
c
c --- UPDATE
c --- V5.0 (011003) to V5.651 (090511)  (DGS)
c        - Reformat date reported at end of run
c
c --- INPUTS:
c       Common block /QA/
c          rdate, rtime, rcpu
c       Parameters: IO6, IOMESG
c
c --- OUTPUT:  none
c
c --- FIN called by:  MAIN
c --- FIN calls:      DATETM, JULDAY, DELTT, FMT_DATE
c----------------------------------------------------------------------
c
c --- include parameters
      include 'params.r62'
      include 'qa.r62'
c
      character*8 rtime2
      character*10 rdate2
      character*12 rdate12
c
      write(iomesg,*)'TERMINATION PHASE'
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
c
         read(rdate2(1:2),10)imo2
         read(rdate2(4:5),10)iday2
         read(rdate2(7:10),'(i4)')iyr2
         call julday(io6,iyr2,imo2,iday2,ijul2)
c
c ---    compute no. hours from beg. of first hour of run to
c ---    ending hour of ending day of the run
         call deltt(iyr1,ijul1,ihr1,iyr2,ijul2,ihr2,idelhr)
c
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

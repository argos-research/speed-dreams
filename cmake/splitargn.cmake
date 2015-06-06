############################################################################
#
#   file        : split_argn.cmake
#   copyright   : (C) 2008 by Mart Kelder
#   web         : www.speed-dreams.org 
#   version     : $Id: splitargn.cmake 4943 2012-09-14 18:39:40Z pouillot $
#
############################################################################

############################################################################
#                                                                          #
#   This program is free software; you can redistribute it and/or modify   #
#   it under the terms of the GNU General Public License as published by   #
#   the Free Software Foundation; either version 2 of the License, or      #
#   (at your option) any later version.                                    #
#                                                                          #
############################################################################

# @file     Generic macro argument parser
# @author   Mart Kelder
# @version  $Id: splitargn.cmake 4943 2012-09-14 18:39:40Z pouillot $

# Internal use only : Get the list of specified keywords
MACRO(SPLIT_ARGN_KEYWORDS SAK_RESULT)

	SET(SAK_FIRST TRUE)
	SET(SAK_FINISHED FALSE)

	FOREACH(SAK_ITEM ${ARGN})
		IF(NOT SAK_FINISHED)
			IF(${SAK_ITEM} STREQUAL ARGUMENTS)
				SET(SAK_FINISHED TRUE)
			ELSE(${SAK_ITEM} STREQUAL ARGUMENTS)
				STRING(REPLACE "," ";" SAK_ITEMLIST ${SAK_ITEM})
				LIST(GET SAK_ITEMLIST 0 SAK_KEYWORD)
				IF(SAK_FIRST)
					SET(${SAK_RESULT} ${SAK_KEYWORD})
					SET(SAK_FIRST FALSE)
				ELSE(SAK_FIRST)
					SET(${SAK_RESULT} ${${SAK_RESULT}} ${SAK_KEYWORD})
				ENDIF(SAK_FIRST)
			ENDIF(${SAK_ITEM} STREQUAL ARGUMENTS)
		ENDIF(NOT SAK_FINISHED)
	ENDFOREACH(SAK_ITEM ${ARGN})

ENDMACRO(SPLIT_ARGN_KEYWORDS SAK_RESULT)

# Internal use only : Initialize output vars for each keyword
MACRO(SPLIT_ARGN_CLEAN_VARS)

	SET(SACI_FINISHED FALSE)

	FOREACH(SACI_ITEM ${ARGN})
		IF(NOT SACI_FINISHED)
			IF(${SACI_ITEM} STREQUAL ARGUMENTS)
				SET(SACI_FINISHED TRUE)
			ELSE(${SACI_ITEM} STREQUAL ARGUMENTS)
				STRING(REPLACE "," ";" SACI_ITEMLIST ${SACI_ITEM})
				LIST(GET SACI_ITEMLIST 3 SACI_ISVARNAME)
				SET(${SACI_ISVARNAME} FALSE)
				LIST(GET SACI_ITEMLIST 4 SACI_VARNAME)
				SET(${SACI_VARNAME})
			ENDIF(${SACI_ITEM} STREQUAL ARGUMENTS)
		ENDIF(NOT SACI_FINISHED)
	ENDFOREACH(SACI_ITEM ${ARGN})

ENDMACRO(SPLIT_ARGN_CLEAN_VARS)

# Internal use only : Parse args for a given keyword
MACRO(SPLIT_ARGN_GET_KEYWORD SAG_KEYWORD SAG_FOUND SAG_KEYWORDNAME SAG_MINCOUNT SAG_MAXCOUNT SAG_ISVARNAME SAG_VARNAME)

	SET(${SAG_FOUND} FALSE)
	SET(SAG_FINISHED FALSE)

	FOREACH(SAG_ITEM ${ARGN})
		IF(NOT SAG_FINISHED)
			IF(${SAG_ITEM} STREQUAL ARGUMENTS)
				SET(SAG_FINISHED TRUE)
			ELSE(${SAG_ITEM} STREQUAL ARGUMENTS)
				STRING(REPLACE "," ";" SAG_ITEMLIST ${SAG_ITEM})
				LIST(GET SAG_ITEMLIST 0 SAG_ITEMKEYWORD)
				IF(${SAG_KEYWORD} STREQUAL ${SAG_ITEMKEYWORD})
					SET(${SAG_FOUND} TRUE)
					LIST(GET SAG_ITEMLIST 0 ${SAG_KEYWORDNAME})
					LIST(GET SAG_ITEMLIST 1 ${SAG_MINCOUNT})
					LIST(GET SAG_ITEMLIST 2 ${SAG_MAXCOUNT})
					LIST(GET SAG_ITEMLIST 3 ${SAG_ISVARNAME})
					LIST(GET SAG_ITEMLIST 4 ${SAG_VARNAME})
				ENDIF(${SAG_KEYWORD} STREQUAL ${SAG_ITEMKEYWORD})
			ENDIF(${SAG_ITEM} STREQUAL ARGUMENTS)
		ENDIF(NOT SAG_FINISHED)
	ENDFOREACH(SAG_ITEM ${ARGN})

ENDMACRO(SPLIT_ARGN_GET_KEYWORD SAG_KEYWORD SAG_FOUND SAG_KEYWORDNAME SAG_MINCOUNT SAG_MAXCOUNT SAG_ISVARNAME SAG_VARNAME)

# Internal use only : Test if a string is a keyword
MACRO(SPLIT_ARGN_IS_KEYWORD SAG_KEYWORDS SAG_ITEM SAG_RESULT)

	LIST(FIND ${SAG_KEYWORDS} ${SAG_ITEM} SAG_FIND)
	IF(${SAG_FIND} LESS 0)
		SET(${SAG_RESULT} FALSE)
	ELSE(${SAG_FIND} LESS 0)
		SET(${SAG_RESULT} TRUE)
	ENDIF(${SAG_FIND} LESS 0)

ENDMACRO(SPLIT_ARGN_IS_KEYWORD SAG_KEYWORDS SAG_ITEM)

# The argument parser
MACRO(SPLIT_ARGN)

	SPLIT_ARGN_KEYWORDS(SA_KEYWORDS ${ARGN})
	SPLIT_ARGN_CLEAN_VARS(${ARGN})
	SET(SA_EXPECT_KEYWORD TRUE)
	SET(SA_EXPECT_VALUE FALSE)
	SET(SA_START FALSE)

	#Loop through list
	FOREACH(SA_ITEM ${ARGN})

		IF(NOT SA_START)

			IF(${SA_ITEM} STREQUAL ARGUMENTS)
				SET(SA_START TRUE)
			ENDIF(${SA_ITEM} STREQUAL ARGUMENTS)

		ELSE(NOT SA_START)

			#Check if the current item is a keyword
			SPLIT_ARGN_IS_KEYWORD(SA_KEYWORDS ${SA_ITEM} SA_ISKEYWORD)

			IF(SA_ISKEYWORD)

				#Check if it is what we expect
				IF(NOT SA_EXPECT_KEYWORD)
					MESSAGE(ERROR "Unexpected keyword in SPLIT_ARGN, got \"${SA_ITEM}\".")
				ELSE(NOT SA_EXPECT_KEYWORD)
					#Save information about the keyword
					SPLIT_ARGN_GET_KEYWORD(${SA_ITEM} SA_KEYWORD_FOUND SA_CUR_KEYWORD SA_CUR_MINCOUNT SA_CUR_MAXCOUNT SA_CUR_ISVARNAME SA_CUR_VARNAME ${ARGN})
					IF(SA_KEYWORD_FOUND)
						SET(SA_CUR_INDEX 0)
						SET(${SA_CUR_VARNAME})
						SET(${SA_CUR_ISVARNAME} FALSE)
					ELSE(SA_KEYWORD_FOUND)
						MESSAGE(ERROR "Got keyword, but didn't found information about \"${SA_ITEM}\"")
					ENDIF(SA_KEYWORD_FOUND)
				ENDIF(NOT SA_EXPECT_KEYWORD)

			ELSE(SA_ISKEYWORD)

				#Check if it is what we expect
				IF(NOT SA_EXPECT_VALUE)
					MESSAGE(ERROR "Unexpected value in SPLIT_ARGN, got \"${SA_ITEM}\".")
				ELSE(NOT SA_EXPECT_VALUE)
					#Save this item to the list
					SET(${SA_CUR_VARNAME} ${${SA_CUR_VARNAME}} ${SA_ITEM})
	
					#Increase index
					MATH(EXPR SA_CUR_INDEX_TMP "${SA_CUR_INDEX}+1")
					SET(SA_CUR_INDEX ${SA_CUR_INDEX_TMP})
				ENDIF(NOT SA_EXPECT_VALUE)
			ENDIF(SA_ISKEYWORD)
	
			#Check what to expect from the next element
			IF(${SA_CUR_INDEX} LESS ${SA_CUR_MINCOUNT})
				SET(SA_EXPECT_KEYWORD FALSE)
				SET(SA_EXPECT_VALUE TRUE)
			ELSE(${SA_CUR_INDEX} LESS ${SA_CUR_MINCOUNT})
				SET(${SA_CUR_ISVARNAME} TRUE)
				SET(SA_EXPECT_KEYWORD TRUE)
				IF(${SA_CUR_INDEX} LESS ${SA_CUR_MAXCOUNT} OR ${SA_CUR_MAXCOUNT} LESS 0)
					SET(SA_EXPECT_VALUE TRUE)
				ELSE(${SA_CUR_INDEX} LESS ${SA_CUR_MAXCOUNT} OR ${SA_CUR_MAXCOUNT} LESS 0)
					SET(SA_EXPECT_VALUE FALSE)
				ENDIF(${SA_CUR_INDEX} LESS ${SA_CUR_MAXCOUNT} OR ${SA_CUR_MAXCOUNT} LESS 0)
			ENDIF(${SA_CUR_INDEX} LESS ${SA_CUR_MINCOUNT})
		ENDIF(NOT SA_START)

	ENDFOREACH(SA_ITEM ${ARGN})
	
	IF(SA_START AND NOT SA_EXPECT_KEYWORD)
		#Didn't get all the expected values of the last keyword
		MESSAGE(ERROR "Missing values at the end in SPLIT_ARGN.")
	ENDIF(SA_START AND NOT SA_EXPECT_KEYWORD)

ENDMACRO(SPLIT_ARGN)


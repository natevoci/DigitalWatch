;/**
; *	DigitalWatch.nsi
; *	Copyright (C) 2004
; *
; *	This file is part of DigitalWatch, a free DTV watching and recording
; *	program for the VisionPlus DVB-T.
; *
; *	DigitalWatch is free software; you can redistribute it and/or modify
; *	it under the terms of the GNU General Public License as published by
; *	the Free Software Foundation; either version 2 of the License, or
; *	(at your option) any later version.
; *
; *	DigitalWatch is distributed in the hope that it will be useful,
; *	but WITHOUT ANY WARRANTY; without even the implied warranty of
; *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *	GNU General Public License for more details.
; *
; *	You should have received a copy of the GNU General Public License
; *	along with DigitalWatch; if not, write to the Free Software
; *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
; */
;
;--------------------------------

SetCompressor lzma

!include "Sections.nsh"

!define VersionShort "0.701"
!define VersionLong "0.70.0.1"
!define Version "0701"

; The name of the installer
Name "DigitalWatch ${VersionShort}"

; Put the output file out folder above the digitalwatch folder.
OutFile "..\..\..\DW${Version}.exe"

; The default installation directory
InstallDir $PROGRAMFILES\DigitalWatch\DW${Version}

;Load the english language file
LoadLanguageFile "${NSISDIR}\Contrib\Language Files\English.nlf"
;--------------------------------
;Version Information

	VIProductVersion "${VersionLong}"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "DigitalWatch"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" ""
	VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" ""
	VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
	VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" ""
	VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" ""
	VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VersionShort}"

;--------------------------------

LicenseText "DigitalWatch is released under the GPL open source license."
LicenseData "..\..\COPYING"

;--------------------------------

; Pages

Page license
Page directory
Page components
Page instfiles

;--------------------------------

; The stuff to install
Section "DigitalWatch ${VersionShort} (required)"

	;Set as required
	SectionIn RO

	; Set output path to the installation directory.
	SetOutPath $INSTDIR
	File "DigitalWatch.exe"
	File "Settings.ini"
	File "VideoDecoders.ini"
	File "AudioDecoders.ini"
	File "OSD.ini"
	File "Resolutions.ini"
	File "Keys.ini"
	File "ControlBar.ini"
	File /oname="Channels.ini" "blank channels.ini"
	File "channels Functions Example.ini"
	File "..\..\..\ScanChannels\Release\ScanChannels.exe"

	SetOutPath $INSTDIR\docs
	File "..\..\docs\ReadMe.txt"
	File "..\..\docs\History.txt"
	File "..\..\docs\Functions.txt"
	File "..\..\docs\Configuration.txt"

	SetOutPath $INSTDIR\images
	File "images\background.bmp"
	File "images\controlbar.bmp"
	File "images\network0.bmp"
	File "images\network1.bmp"
	File "images\network2.bmp"
	File "images\network3.bmp"
	File "images\network4.bmp"
	File "images\network5.bmp"

SectionEnd ; end the section

Section "Create Start Menu Icons"
	SetOutPath $INSTDIR
	CreateDirectory "$SMPROGRAMS\DigitalWatch ${VersionShort}"
	CreateShortCut "$SMPROGRAMS\DigitalWatch ${VersionShort}\DigitalWatch ${VersionShort}.lnk" "$INSTDIR\DigitalWatch.exe" "" "$INSTDIR\DigitalWatch.exe" 0
	CreateShortCut "$SMPROGRAMS\DigitalWatch ${VersionShort}\ScanChannels.lnk" "$INSTDIR\ScanChannels.exe" "" "$INSTDIR\ScanChannels.exe" 0
	SetOutPath $INSTDIR\docs
	CreateDirectory "$SMPROGRAMS\DigitalWatch ${VersionShort}\Documentation"
	CreateShortCut "$SMPROGRAMS\DigitalWatch ${VersionShort}\Documentation\ReadMe.txt.lnk" "$INSTDIR\docs\ReadMe.txt" ""
	CreateShortCut "$SMPROGRAMS\DigitalWatch ${VersionShort}\Documentation\History.txt.lnk" "$INSTDIR\docs\History.txt" ""
	CreateShortCut "$SMPROGRAMS\DigitalWatch ${VersionShort}\Documentation\Functions.txt.lnk" "$INSTDIR\docs\Functions.txt" ""
	CreateShortCut "$SMPROGRAMS\DigitalWatch ${VersionShort}\Documentation\Configuration.txt.lnk" "$INSTDIR\docs\Configuration.txt" ""
SectionEnd

Section /o "Create Desktop Icon"
	SetOutPath $INSTDIR
	CreateShortCut "$DESKTOP\DigitalWatch ${VersionShort}.lnk" "$INSTDIR\DigitalWatch.exe" "" "$INSTDIR\DigitalWatch.exe" 0
SectionEnd

Section /o "Create Quick Launch Icon"
	SetOutPath $INSTDIR
	CreateShortCut "$QUICKLAUNCH\DigitalWatch ${VersionShort}.lnk" "$INSTDIR\DigitalWatch.exe" "" "$INSTDIR\DigitalWatch.exe" 0
SectionEnd

Section "" ; empty string makes it hidden
	MessageBox MB_YESNO|MB_ICONQUESTION "Would you like to view the ReadMe file?" IDNO NoReadMe
	SearchPath $1 notepad.exe
	Exec '"$1" "$INSTDIR\docs\ReadMe.txt"'
	NoReadMe:
SectionEnd


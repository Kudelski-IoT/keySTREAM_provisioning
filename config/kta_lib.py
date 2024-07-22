# coding: utf-8
"""*****************************************************************************
* Copyright (C) 2021 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*****************************************************************************"""
import os
import xml.etree.ElementTree as ET

KTA_LIB_FILE_LIST =r"config\filelist\kta_files.xml"

XML_ATTRIB_NAME = "name"
XML_ATTRIB_DIR  = "dir"
XML_ATTRIB_FILE  = "file"

def ktaSetValue(symbol, event):
    component = symbol.getComponent()
    myBool  = component.getSymbolValue('KTA_LOG_BOOL')  
    
    if myBool == True:
        symbol.setValue(1)
    else:
        symbol.setValue(0)

def AddFile(child,component, strPath, strFileName, strDestPath, strProjectPath, bOverWrite=True,strType="SOURCE",bMarkup=False):
    ktaSourceFile = component.createFileSymbol(strPath.upper(), None)
    ktaSourceFile.setSourcePath(strPath)
    ktaSourceFile.setOverwrite(bOverWrite)
    ktaSourceFile.setOutputName(strFileName)
    ktaSourceFile.setDestPath(strDestPath)
    ktaSourceFile.setProjectPath(strProjectPath)
    ktaSourceFile.setType(strType)
    ktaSourceFile.setMarkup(bMarkup)

    if(".h" in strFileName):
        ktaSourceFile.setType("HEADER")
    elif(".c" in strFileName):
        ktaSourceFile.setType("SOURCE")

    if ".ftl" in strFileName:
        ktaSourceFile.setMarkup(True)
        ktaSourceFile.setOutputName(strFileName[:-len(".ftl")])
    else:
        ktaSourceFile.setMarkup(False)

def AddDir(root,component,strPath, strRelativeFilePath,strProjectPath):
    for child in root:
        NewstrPath = strPath + "/" + str(child.attrib[XML_ATTRIB_NAME])
        if child.tag == XML_ATTRIB_DIR:
            if ((str(child.attrib[XML_ATTRIB_NAME]) == "kta_lib") or (str(child.attrib[XML_ATTRIB_NAME]) == "kta_provisioning")):
                NewstrRelativeFilePath = strRelativeFilePath
                NewstrProjectPath = strProjectPath
            else:
                NewstrRelativeFilePath = strRelativeFilePath +  "/" + str(child.attrib[XML_ATTRIB_NAME])
                NewstrProjectPath = strProjectPath +  "/" + str(child.attrib[XML_ATTRIB_NAME])
            AddDir(child,component,NewstrPath, NewstrRelativeFilePath ,NewstrProjectPath)
        if child.tag == XML_ATTRIB_FILE:
            AddFile(child, component,NewstrPath, str(child.attrib[XML_ATTRIB_NAME]), strRelativeFilePath ,strProjectPath)

def AddKTAFile(component,strPath, projectPath, strXmlFile):
    strXmlFile = strXmlFile.replace("\\", "/")
    try:
        tree = ET.parse(strXmlFile)
        print("parsing Succeeded")
        root = tree.getroot()
        if root is not None:
            print("Root Element:", root.tag)
        else:
            print("Failed to get root element")
    except ET.ParseError as e:
        print("XML parsing failed:", e)

    for child in root:
        if child.tag == "loc":
            NewstrPath = strPath + "/" + str(child.attrib["srcpath"])
            AddDir(child,component,NewstrPath, "library/" + str(child.attrib["dstpath"]),projectPath +  "/" + str(child.attrib["dstpath"]))
        else:
            NewstrPath = strPath + "/" + str(child.attrib[XML_ATTRIB_NAME])
            if child.tag == XML_ATTRIB_DIR:
                AddDir(child,component,NewstrPath, "library/" + str(child.attrib[XML_ATTRIB_NAME]),projectPath +  "/" + str(child.attrib[XML_ATTRIB_NAME]))
            if child.tag == XML_ATTRIB_FILE:
                AddFile(child,component,NewstrPath, str(child.attrib[XML_ATTRIB_NAME]), "library/" + str(child.attrib[XML_ATTRIB_NAME]), projectPath + "/" + str(child.attrib[XML_ATTRIB_NAME]))

def instantiateComponent(ktaComponent):

    print ("KTA Component Instantiated")

    Database.activateComponents(["stdio"])
    Database.activateComponents(["cryptoauthlib"])

    comboValues = ["E_KTALOG_LEVEL_DEBUG", "E_KTALOG_LEVEL_INFO", "E_KTALOG_LEVEL_WARN",
                   "E_KTALOG_LEVEL_ENTRY_EXIT", "E_KTALOG_LEVEL_ERROR", "E_KTALOG_LEVEL_NONE"]
    ktaLogDebugSymbol = ktaComponent.createComboSymbol("KTA_LOG_LEVEL", None, comboValues)
    ktaLogDebugSymbol.setLabel("KTA DEBUG LOG LEVEL")
    ktaLogDebugSymbol.setVisible(True)
    ktaLogDebugSymbol.setDefaultValue("E_KTALOG_LEVEL_NONE")

    ktaSymbol = ktaComponent.createStringSymbol("KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID", None)
    ktaSymbol.setLabel("Enter Fleet Profile Public UID")
    ktaSymbol.setVisible(True)
    ktaSymbol.setDefaultValue("")
    
    configName = Variables.get("__CONFIGURATION_NAME")
    includePath = '../src/config/' + configName + '/library/kta_lib'
    projectPath="config/" + configName + '/library'

    # Adding KTA Files
    AddKTAFile(ktaComponent, "..", projectPath,Module.getPath()+KTA_LIB_FILE_LIST)
    
    # Append the include paths in MPLABX IDE
    ktaSymbol = ktaComponent.createSettingSymbol("KTA_XC32_INCLUDE_DIRS", None)
    ktaSymbol.setCategory("C32")
    ktaSymbol.setKey("extra-include-directories")
    ktaSymbol.setValue( '{0};{0}/../cryptoauthlib/atcacert;{0};{0}/COMMON/logmodule/include;{0}/COMMSTACK/http;{0}/COMMSTACK/http/include;{0}/SOURCE/include;{0}/SOURCE/kta/common/crypto/include;{0}/SOURCE/kta/common/general/include;{0}/SOURCE/kta/common/icpp_parser/include;{0}/SOURCE/kta/common/version/include;{0}/SOURCE/kta/modules/acthandler/include;{0}/SOURCE/kta/modules/cmdhandler/include;{0}/SOURCE/kta/modules/config/include;{0}/SOURCE/kta/modules/reghandler/include;{0}/SOURCE/salapi/include'.format(includePath))
    ktaSymbol.setAppend(True, ';')

    # Add the MACRO's in MPLABX IDE
    ktaSymbol = ktaComponent.createSettingSymbol("KTA_XC32_PREPROCESSOR", None)
    ktaSymbol.setCategory("C32")
    ktaSymbol.setKey("preprocessor-macros")
    ktaSymbol.setValue( 'CLOUD_CONNECT_WITH_CUSTOM_CERTS;OBJECT_MANAGEMENT_FEATURE;CLOUD_CONFIG_AWS')
    ktaSymbol.setAppend(True, ';')

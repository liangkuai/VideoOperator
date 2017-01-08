#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string.h>
#include "vr_ini.h"

using namespace std;

#pragma warning(disable:4996)

#ifndef WIN32
char* strupr(char *str)
{
    // We dont check the ptr because the original also dont do it.
    while(*str != 0)
    {
        if(islower(*str))
        {
            *str = toupper(*str);
        }
        str++;
    }
	return str;
}
#endif

//=========================================================================
// strupr -de-
// -------------------------------------------------------------------------
// Job : String to Uppercase 22.03.2001 Dieter Engelbrecht dieter@wintop.net
//========================================================================

// DONT_HAVE_STRUPR is set when INI_REMOVE_CR is defined


//=========================================================================
//   CIniFile : The constructor
//========================================================================
CIniFile::CIniFile()
{
    m_pEntry = NULL;
    m_pCurEntry = NULL;
    m_result [0] = 0;
    m_pIniFile = NULL;
}

//========================================================================
//   CIniFile : The destructor
//========================================================================
CIniFile::~CIniFile()
{
    FreeAllMem();
}

/*=========================================================================
   CIniFile : GetVersion
   Info     : The version is BCD coded. It maintain the major version in
              the upper 8 bits and the minor in the lower.
              0x0120 means version 1.20
========================================================================*/
UINT CIniFile::GetVersion(void)
{
    return 0x0030;
}

/*=========================================================================
   OpenIniFile
  -------------------------------------------------------------------------
   Job : Opens an ini file or creates a new one if the requested file
         doesnt exists.
========================================================================*/
bool CIniFile::OpenIniFile(CCHR *FileName)
{
    char Str[255];
    char *pStr;
    struct ENTRY *pEntry;

    FreeAllMem();
    int Len;
    if(FileName == NULL)
    {
        return FALSE;
    }
    
    if((m_pIniFile = fopen(FileName, "r")) == NULL)
    {
        return FALSE;
    }

    while(fgets(Str, 255, m_pIniFile) != NULL)
    {
        pStr = strchr(Str, '\n');
        if(pStr != NULL)
        {
            *pStr = 0; 
        }
        
        pEntry = MakeNewEntry();
        if(pEntry == NULL)
        {
            return FALSE;
        }

#ifdef INI_REMOVE_CR
        Len = strlen(Str);
        if(Len > 0)
        {
            if(Str[Len - 1] == '\r')
            {
                Str[Len - 1] = '\0';
            }
        }
#endif

        pEntry->pText = (char*)malloc(strlen(Str) + 1);
        if(pEntry->pText == NULL)
        {
            FreeAllMem();
            return FALSE;
        }
        strcpy(pEntry->pText, Str);

        pStr = strchr(Str, ';');
        if(pStr != NULL)
        { 
            *pStr = 0; 
        }
        
        if((strstr(Str, "[") > 0) && (strstr(Str, "]") > 0)) /* Is Section */
        {
            pEntry->Type = tpSECTION;
        }
        else
        {
            if(strstr(Str, "=") > 0)
            {
                pEntry->Type = tpKEYVALUE;
            }
            else
            {
                pEntry->Type = tpCOMMENT;
            }
        }
        m_pCurEntry = pEntry;
    }
    fclose(m_pIniFile);
    m_pIniFile = NULL;
    return TRUE;
}

/*=========================================================================
   CloseIniFile
  -------------------------------------------------------------------------
   Job : Closes the ini file without any modifications. If you want to
         write the file use WriteIniFile instead.
========================================================================*/
void CIniFile::CloseIniFile(void)
{
    FreeAllMem();
    if(m_pIniFile != NULL)
    {
        fclose(m_pIniFile);
        m_pIniFile = NULL;
    }
}

/*=========================================================================
   WriteIniFile
  -------------------------------------------------------------------------
   Job : Writes the iniFile to the disk and close it. Frees all memory
         allocated by WriteIniFile;
=========================================================================*/
bool CIniFile::WriteIniFile(const char *pFileName)
{
    struct ENTRY *pEntry = m_pEntry;

    if(m_pIniFile != NULL)
    {
        fclose(m_pIniFile);
    }

    if((m_pIniFile = fopen(pFileName, "wb")) == NULL)
    {
        FreeAllMem();
        return FALSE;
    }

    while (pEntry != NULL)
    {
        if(pEntry->Type != tpNULL)
        {
#ifdef INI_REMOVE_CR
        fprintf(m_pIniFile, "%s\n", pEntry->pText);
#else
        fprintf(m_pIniFile, "%s\r\n", pEntry->pText);
#endif
        }
        pEntry = pEntry->pNext;
    }

    fclose(m_pIniFile);
    m_pIniFile = NULL;
    return TRUE;
}

/*=========================================================================
   WriteString : Writes a string to the ini file
=========================================================================*/
void CIniFile::WriteString(CCHR *pSection, CCHR *pKey, CCHR *pValue)
{
    EFIND List;
    char Str[255];

    if(ArePtrValid(pSection, pKey, pValue) == FALSE)
    {
        return;
    }
    
    if(FindKey(pSection, pKey, &List) == TRUE)
    {
        sprintf(Str, "%s=%s%s", List.KeyText, pValue, List.Comment);
        FreeMem(List.pKey->pText);
        List.pKey->pText = (char*)malloc(strlen(Str) + 1);
        strcpy(List.pKey->pText, Str);
    }
    else
    {
        if((List.pSec != NULL) && (List.pKey == NULL)) /*section exist, Key not*/
        {
            AddKey(List.pSec, pKey, pValue);
        }
        else
        {
            AddSectionAndKey(pSection, pKey, pValue);
        }
    }
}

/*=========================================================================
   WriteBool : Writes a boolean to the ini file
=========================================================================*/
void CIniFile::WriteBool(CCHR *pSection, CCHR *pKey, bool Value)
{
    char Val[2] = {'0', 0};

    if(Value != 0)
    { 
        Val [0] = '1'; 
    }
    WriteString(pSection, pKey, Val);
}

/*=========================================================================
   WriteInt : Writes an integer to the ini file
=========================================================================*/
void CIniFile::WriteInt(CCHR *pSection, CCHR *pKey, int Value)
{
    char Val[12]; /*32bit maximum + sign + \0*/

    sprintf(Val, "%d", Value);
    WriteString(pSection, pKey, Val);
}

/*=========================================================================
   WriteDouble : Writes a double to the ini file
=========================================================================*/
void CIniFile::WriteDouble(CCHR *pSection, CCHR *pKey, double Value)
{
    char Val[32]; /* DDDDDDDDDDDDDDD+E308\0 */
    
    sprintf(Val, "%1.10lE", Value);
    WriteString(pSection, pKey, Val);
}

/*=========================================================================
   ReadString : Reads a string from the ini file
=========================================================================*/
CCHR *CIniFile::ReadString(CCHR *pSection, CCHR *pKey, CCHR *pDefault)
{
    EFIND List;
    
    if(ArePtrValid(pSection, pKey, pDefault) == FALSE)
    {
        return pDefault;
    }
    
    if(FindKey(pSection, pKey, &List) == TRUE)
    {
        strcpy(m_result, List.ValText);
        return m_result;
    }
    
    return pDefault;
}

/*=========================================================================
   ReadBool : Reads a boolean from the ini file
=========================================================================*/
bool CIniFile::ReadBool(CCHR *pSection, CCHR *pKey, bool Default)
{
    char Val[2] = {"0"};
    
    if(Default != 0)
    { 
        Val[0] = '1'; 
    }
    return (atoi(ReadString(pSection, pKey, Val)) ? 1 : 0); /*Only allow 0 or 1*/
}

/*=========================================================================
   ReadInt : Reads a integer from the ini file
*========================================================================*/
int CIniFile::ReadInt(CCHR *pSection, CCHR *pKey, int Default)
{
    char Val[12];

    sprintf(Val,"%d", Default);
    return (atoi(ReadString(pSection, pKey, Val)));
}

/*=========================================================================
   ReadDouble : Reads a double from the ini file
*========================================================================*/
double CIniFile::ReadDouble(CCHR *pSection, CCHR *pKey, double Default)
{
    double Val;

    sprintf(m_result, "%1.10lE", Default);
    sscanf(ReadString(pSection, pKey, m_result), "%lE", &Val);
    return Val;
}

/*=========================================================================
   DeleteKey : Deletes an entry from the ini file
*========================================================================*/
bool CIniFile::DeleteKey(CCHR *pSection, CCHR *pKey)
{
    EFIND List;
    struct ENTRY *pPrev;
    struct ENTRY *pNext;

    if(FindKey(pSection, pKey, &List) == TRUE)
    {
        pPrev = List.pKey->pPrev;
        pNext = List.pKey->pNext;
        if(pPrev)
        {
            pPrev->pNext=pNext;
        }
        if(pNext)
        { 
            pNext->pPrev=pPrev;
        }
        FreeMem(List.pKey->pText);
        FreeMem(List.pKey);
        return TRUE;
    }
    return FALSE;
}

/* Here we start with our helper functions */

void CIniFile::FreeMem(void *pPtr)
{
    if(pPtr != NULL)
    {
        free(pPtr);
    }
}

void CIniFile::FreeAllMem(void)
{
    struct ENTRY *pEntry;
    struct ENTRY *pNextEntry;
    pEntry = m_pEntry;
    while(1)
    {
        if(pEntry == NULL)
        {
            break;
        }
        pNextEntry = pEntry->pNext;
        FreeMem(pEntry->pText);  /* Frees the pointer if not NULL */
        FreeMem(pEntry);
        pEntry = pNextEntry;
    }
    m_pEntry = NULL;
    m_pCurEntry = NULL;
}

struct ENTRY *CIniFile::FindSection(CCHR *pSection)
{
    char Sec[130];
    char iSec[130];
    struct ENTRY *pEntry;
    sprintf(Sec, "[%s]", pSection);
    strupr(Sec);
    pEntry = m_pEntry;  /*Get a pointer to the first Entry*/
    while(pEntry != NULL)
    {
        if(pEntry->Type == tpSECTION)
        {
            strcpy(iSec, pEntry->pText);
            strupr(iSec);
            if(strcmp(Sec, iSec) == 0)
            {
                return pEntry;
            }
        }
        pEntry = pEntry->pNext;
    }
    return NULL;
}

bool CIniFile::FindKey(CCHR *pSection, CCHR *pKey, EFIND *pList)
{
    char Search[130];
    char Found[130];
    char Text[255];
    char *pText;
    struct ENTRY *pEntry;
    pList->pSec = NULL;
    pList->pKey = NULL;
    pEntry = FindSection(pSection);

    if(pEntry == NULL)
    {
        return FALSE;
    }
    
    pList->pSec = pEntry;
    pList->KeyText[0] = 0;
    pList->ValText[0] = 0;
    pList->Comment[0] = 0;
    pEntry = pEntry->pNext;

    if(pEntry == NULL)
    {
        return FALSE;
    }
    sprintf(Search, "%s", pKey);

    strupr(Search);
    
    while (pEntry != NULL)
    {
        if((pEntry->Type == tpSECTION) 
            || (pEntry->Type == tpNULL))  /*Stop after next section or EOF*/
        {
            return FALSE;
        }
        
        if(pEntry->Type == tpKEYVALUE)
        {
            strcpy(Text, pEntry->pText);
            pText = strchr(Text, ';');
            //modify by qianzhenghua 2006-6-26
            if(pText != NULL)
            {
                strcpy(pList->Comment, pText);
                *pText = 0;
            }
            else
            {
                strcpy(pList->Comment, "");
            }
            pText = strchr(Text, '=');
            if(pText != NULL)
            {
                *pText = 0;
                strcpy(pList->KeyText, Text);
                strcpy(Found, Text);
                *pText = '=';
                strupr(Found);
                if(strcmp(Found, Search) == 0)
                {
                    strcpy(pList->ValText, pText + 1);
                    pList->pKey = pEntry;
                    return TRUE;
                }
            }
        }
        pEntry = pEntry->pNext;
    }
    return false;
}

bool CIniFile::AddItem(char Type, CCHR *pText)
{
    struct ENTRY *pEntry = MakeNewEntry();

    if(pEntry == NULL)
    {
        return FALSE; 
    }
    pEntry->Type = Type;
    pEntry->pText = (char*)malloc(strlen(pText) + 1);
    if(pEntry->pText == NULL)
    {
        free(pEntry);
        return FALSE;
    }
    strcpy(pEntry->pText, pText);
    pEntry->pNext = NULL;
    if(m_pCurEntry != NULL)
    { 
        m_pCurEntry->pNext = pEntry; 
    }
    m_pCurEntry = pEntry;
    return TRUE;
}

bool CIniFile::AddItemAt(struct ENTRY *pEntryAt, char Mode, CCHR *pText)
{
    struct ENTRY *pNewEntry;

    if(pEntryAt == NULL)
    {
        return FALSE;
    }
    pNewEntry = (struct ENTRY*)malloc(sizeof(ENTRY));
    if(pNewEntry == NULL)
    {
        return FALSE;
    }
    pNewEntry->pText = (char*)malloc(strlen(pText) + 1);
    if(pNewEntry->pText == NULL)
    {
        free(pNewEntry);
        return FALSE;
    }
    strcpy(pNewEntry->pText, pText);
    if(pEntryAt->pNext == NULL)  /* No following nodes. */
    {
        pEntryAt->pNext = pNewEntry;
        pNewEntry->pNext = NULL;
    }
    else
    {
        pNewEntry->pNext = pEntryAt->pNext;
        pEntryAt->pNext = pNewEntry;
    }
    pNewEntry->pPrev = pEntryAt;
    pNewEntry->Type = Mode;
    return TRUE;
}

bool CIniFile::AddSectionAndKey(CCHR *pSection, CCHR *pKey, CCHR *pValue)
{
    char Text[255];
    sprintf(Text, "[%s]", pSection);
    if(AddItem (tpSECTION, Text) == FALSE)
    {
        return FALSE;
    }
    sprintf(Text, "%s=%s", pKey, pValue);
    return AddItem(tpKEYVALUE, Text) ? 1 : 0;
}

void CIniFile::AddKey(struct ENTRY *pSecEntry, CCHR *pKey, CCHR *pValue)
{
    char Text[255];
    sprintf(Text, "%s=%s", pKey, pValue);
    AddItemAt(pSecEntry, tpKEYVALUE, Text);
}

struct ENTRY *CIniFile::MakeNewEntry(void)
{
    struct ENTRY *pEntry;
    pEntry = (struct ENTRY *)malloc(sizeof(ENTRY));
    if(pEntry == NULL)
    {
        FreeAllMem();
        return NULL;
    }
    if(m_pEntry == NULL)
    {
        m_pEntry = pEntry;
    }
    pEntry->Type = tpNULL;
    pEntry->pPrev = m_pCurEntry;
    pEntry->pNext = NULL;
    pEntry->pText = NULL;
    if(m_pCurEntry != NULL)
    {
        m_pCurEntry->pNext = pEntry;
    }
    return pEntry;
}

#if 0
int main(int argc, char* argv[])
{
    CIniFile iFile;	
    iFile.OpenIniFile  ("SystemParam.ini");

	/*
    iFile.WriteString  ("Test", "Name", "OverWrittenValue");
    iFile.WriteString  ("Test", "rrrrort", "COM1");
    iFile.WriteString  ("Test", "User", "James Brown");
    iFile.WriteString  ("Configuration", "eDriver", "MBM2.VXD");
    iFile.WriteString  ("Configuration", "Wrap", "LPT.VXD");
    iFile.WriteInt     ("IO-Port", "Com", 10);
    iFile.WriteBool    ("IO-Port", "IsValid", 0);
    iFile.WriteDouble  ("TheMoney", "TheMoney", 67892.00241);
    //iFile.WriteIniFile ("test.ini");	
    //iFile.CloseIniFile ();


    //iFile.OpenIniFile  ("test.ini");
    iFile.WriteString  ("uuuuuuuuu", "uiiiiiiit", "ooooooooo1");
    iFile.WriteIniFile ("test.ini");	
    iFile.CloseIniFile ();
    //iFile.DeleteKey    ("Test"	  , "ToDelete");


    //iFile.WriteIniFile ("test.ini"); 
	iFile.OpenIniFile("test.ini");
    printf ("[Test] Name = %s\n", iFile.ReadString ("Test", "Name", "notNULL"));
	cout << iFile.ReadString("Test", "Name", "notNULL") << endl;
    printf ("[Test] Port = %s\n", iFile.ReadString ("Test", "rrrrort", "NotFound"));
    printf ("[Test] User = %s\n", iFile.ReadString ("Test", "User", "NotFound"));
    printf ("[Configuration] eDriver = %s\n", iFile.ReadString ("Configuration", "eDriver", "NotFound"));
    printf ("[Configuration] Wrap = %s\n", iFile.ReadString ("Configuration", "Wrap", "NotFound"));
    printf ("[IO-Port] Com = %d\n", iFile.ReadInt ("IO-Port", "Com", 66));
    if(iFile.ReadInt ("IO-Port", "Com", 66))  printf("\n\ngggggggggggggggggggggggg\n\n");
    printf ("[IO-Port] IsValid = %d\n", iFile.ReadBool ("IO-Port", "IsValid", 0));
    printf ("[TheMoney] TheMoney = %1.10lf\n", iFile.ReadDouble ("TheMoney", "TheMoney", 111));
    iFile.CloseIniFile ();
	*/
	const char* p = iFile.ReadString("������Ϣ", "IP", "");

	printf(p);
    return 0; 
}
#endif

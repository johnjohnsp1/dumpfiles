// dumpfiles.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <cstdlib>
#include <cstdio>
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <Shlwapi.h>

// Redefine the API by adding the export attribute.
#undef DECLARE_API
#define DECLARE_API(s)                             \
    CPPMOD __declspec(dllexport) VOID              \
    s(                                             \
        HANDLE                 hCurrentProcess,    \
        HANDLE                 hCurrentThread,     \
        ULONG                  dwCurrentPc,        \
        ULONG                  dwProcessor,        \
        PCSTR                  args                \
     )

#undef DECLARE_API32
#define DECLARE_API32(s)                           \
    CPPMOD __declspec(dllexport) VOID              \
    s(                                             \
        HANDLE                 hCurrentProcess,    \
        HANDLE                 hCurrentThread,     \
        ULONG                  dwCurrentPc,        \
        ULONG                  dwProcessor,        \
        PCSTR                  args                \
     )

#undef DECLARE_API64
#define DECLARE_API64(s)                           \
    CPPMOD __declspec(dllexport) VOID              \
    s(                                             \
        HANDLE                 hCurrentProcess,    \
        HANDLE                 hCurrentThread,     \
        ULONG64                dwCurrentPc,        \
        ULONG                  dwProcessor,        \
        PCSTR                  args                \
     )

#include <ntverp.h>

//
// globals
//
EXT_API_VERSION ApiVersion = { (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff), EXT_API_VERSION_NUMBER64, 0 };
WINDBG_EXTENSION_APIS ExtensionApis;
ULONG SavedMajorVersion;
ULONG SavedMinorVersion;

BOOL extensionInit();

extern "C"
VOID
__declspec(dllexport)
WinDbgExtensionDllInit(
PWINDBG_EXTENSION_APIS lpExtensionApis,
USHORT MajorVersion,
USHORT MinorVersion
) {
	ExtensionApis = *lpExtensionApis;

	SavedMajorVersion = MajorVersion;
	SavedMinorVersion = MinorVersion;

	extensionInit();
	return;
}

extern "C"
LPEXT_API_VERSION
__declspec(dllexport)
ExtensionApiVersion(
VOID
) {
	//
	// ExtensionApiVersion should return EXT_API_VERSION_NUMBER64 in order for APIs
	// to recognize 64 bit addresses.  KDEXT_64BIT also has to be defined before including
	// wdbgexts.h to get 64 bit headers for WINDBG_EXTENSION_APIS
	//
	return &ApiVersion;
}

//
// Routine called by debugger after load
//

extern "C"
VOID
__declspec(dllexport)
CheckVersion(
VOID
) {
	return;
}

/*
 *
 *
 *
 *
 *
 *
 *
 */

#define DEBUG 0

#if DEBUG
#define ddprintf(fmt, ...) dprintf(fmt, __VA_ARGS__)
#else
#define ddprintf(fmt, ...)
#endif

ULONG g_NextSubsectionOffset;
ULONG g_PtesInSubsectionOffset;
ULONG g_SubsectionBaseOffset;
ULONG g_StartingSectorOffset;
ULONG g_SectionObjectPointerOffset;
ULONG g_FileNameOffset;
ULONG g_DataSectionObjectOffset;
ULONG g_ImageSectionObjectOffset;
ULONG g_SubsectionOffset;
ULONG g_NumberOfPfnReferencesOffset;
ULONG g_SegmentOffset;
ULONG g_SharedCacheMapOffset;
ULONG g_InitialVacbsOffset[4];

BOOL extensionInit(){
	int rc = GetFieldOffset("_FILE_OBJECT", "SectionObjectPointer", &g_SectionObjectPointerOffset);
	ddprintf("SectionObjectPointerOffset %d %p\n", rc, g_SectionObjectPointerOffset);
	if (rc != 0){
		dprintf("Failed to get SectionObjectPointerOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_FILE_OBJECT", "FileName", &g_FileNameOffset);
	ddprintf("FileNameOffset %d %p\n", rc, g_FileNameOffset);
	if (rc != 0){
		dprintf("Failed to get FileNameOffset");
		return FALSE;
	}
	
	rc = GetFieldOffset("_SECTION_OBJECT_POINTERS", "DataSectionObject", &g_DataSectionObjectOffset);
	ddprintf("DataSectionObjectOffset %d %p\n", rc, g_DataSectionObjectOffset);
	if (rc != 0){
		dprintf("Failed to get DataSectionObjectOffset");
		return FALSE;
	}
	
	rc = GetFieldOffset("_SECTION_OBJECT_POINTERS", "ImageSectionObject", &g_ImageSectionObjectOffset);
	ddprintf("ImageSectionObjectOffset %d %p\n", rc, g_ImageSectionObjectOffset);
	if (rc != 0){
		dprintf("Failed to get ImageSectionObjectOffset");
		return FALSE;
	}
	
	g_SubsectionOffset = GetTypeSize("nt!_CONTROL_AREA");
	ddprintf("SubsectionOffset %p\n", g_SubsectionOffset);
	if (rc != 0){
		dprintf("Failed to get SubsectionOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_SUBSECTION", "SubsectionBase", &g_SubsectionBaseOffset);
	ddprintf("SubsectionBaseOffset %d %p\n", rc, g_SubsectionBaseOffset);
	if (rc != 0){
		dprintf("Failed to get SubsectionBaseOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_CONTROL_AREA", "NumberOfPfnReferences", &g_NumberOfPfnReferencesOffset);
	ddprintf("NumberOfPfnReferencesOffset %d %p\n", rc, g_NumberOfPfnReferencesOffset);
	if (rc != 0){
		dprintf("Failed to get NumberOfPfnReferencesOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_CONTROL_AREA", "Segment", &g_SegmentOffset);
	ddprintf("SegmentOffset %d %p\n", rc, g_SegmentOffset);
	if (rc != 0){
		dprintf("Failed to get SegmentOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_SUBSECTION", "PtesInSubsection", &g_PtesInSubsectionOffset);
	ddprintf("PtesInSubsectionOffset %d %p\n", rc, g_PtesInSubsectionOffset);
	if (rc != 0){
		dprintf("Failed to get PtesInSubsectionOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_SUBSECTION", "NextSubsection", &g_NextSubsectionOffset);
	ddprintf("NextSubsectionOffset %d %p\n", rc, g_NextSubsectionOffset);
	if (rc != 0){
		dprintf("Failed to get NextSubsectionOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_SUBSECTION", "StartingSector", &g_StartingSectorOffset);
	ddprintf("StartingSectorOffset %d %p\n", rc, g_StartingSectorOffset);
	if (rc != 0){
		dprintf("Failed to get StartingSectorOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_SECTION_OBJECT_POINTERS", "SharedCacheMap", &g_SharedCacheMapOffset);
	ddprintf("SharedCacheMapOffset %d %p\n", rc, g_SharedCacheMapOffset);
	if (rc != 0){
		dprintf("Failed to get SharedCacheMapOffset");
		return FALSE;
	}

	rc = GetFieldOffset("_SHARED_CACHE_MAP", "InitialVacbs[0]", &g_InitialVacbsOffset[0]);
	rc = GetFieldOffset("_SHARED_CACHE_MAP", "InitialVacbs[1]", &g_InitialVacbsOffset[1]);
	rc = GetFieldOffset("_SHARED_CACHE_MAP", "InitialVacbs[2]", &g_InitialVacbsOffset[2]);
	rc = GetFieldOffset("_SHARED_CACHE_MAP", "InitialVacbs[3]", &g_InitialVacbsOffset[3]);
	ddprintf("InitialVacbsOffset[3] %d %p\n", rc, g_InitialVacbsOffset[3]);
	if (rc != 0){
		dprintf("Failed to get InitialVacbsOffset[3]");
		return FALSE;
	}
	return TRUE;
}

DECLARE_API(dumpfileshelp)
{
	dprintf("!dumpfiles.dump fileobj");
}

BOOL DirectoryExists(LPCWSTR szPath){
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL DirectoryCreate(wchar_t *path){
	wchar_t folder[MAX_PATH];
	wchar_t *end;
	ZeroMemory(folder, MAX_PATH * sizeof(wchar_t));

	end = wcschr(path, L'\\');

	while (end != NULL){
		wcsncpy_s(folder, path, end - path + 1);
		if (!CreateDirectory(folder, NULL)){
			DWORD err = GetLastError();

			if (err != ERROR_ALREADY_EXISTS){
				// do whatever handling you'd like
			}
		}
		end = wcschr(++end, L'\\');
	}
	return TRUE;
}





bool dumpSubSection(HANDLE hOutputFile, ULONG_PTR Subsection){
	while (Subsection > 0){
		ddprintf("Subsection %p\n", Subsection);

		UINT32 PtesInSubsection;
		int rc = ReadMemory(Subsection + g_PtesInSubsectionOffset, &PtesInSubsection, sizeof(PtesInSubsection), NULL);
		ddprintf("PtesInSubsection %d %p\n", rc, PtesInSubsection);
		if (rc != 1){
			return false;
		}

		UINT32 StartingSector;
		rc = ReadMemory(Subsection + g_StartingSectorOffset, &StartingSector, sizeof(StartingSector), NULL);
		ddprintf("StartingSector %d %p\n", rc, StartingSector);
		if (rc != 1){
			return false;
		}

		ULONG_PTR SubsectionBase;
		rc = ReadPointer(Subsection + g_SubsectionBaseOffset, &SubsectionBase);
		ddprintf("SubsectionBase %d %p\n", rc, SubsectionBase);
		if (rc != 1){
			return false;
		}

		for (UINT32 i = 0; i<PtesInSubsection; i++){
			ULONG_PTR PhysicalAddress;
			rc = ReadMemory(SubsectionBase + i*sizeof(ULONG_PTR), &PhysicalAddress, sizeof(PhysicalAddress), NULL);
			if (rc != 1){
				return false;
			}
			//dprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
			if (PhysicalAddress & 0x1){ //Is valid
				//TODO : function dumpPhysicalBlaBla...
				PhysicalAddress = PhysicalAddress & 0x00007FFFFFFFF000;
				ddprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
				char tmpBuffer[4096];
				ULONG readBytes;
				ReadPhysical(PhysicalAddress, tmpBuffer, sizeof(tmpBuffer), &readBytes);
				if (readBytes == sizeof(tmpBuffer)){
					LONG inFileOffset = (StartingSector * 0x200) + (i * 0x1000/*TODO: PAGE_SIZE*/);
					SetFilePointer(hOutputFile, inFileOffset, NULL, FILE_BEGIN);
					ULONG writtenBytes;
					WriteFile(hOutputFile, tmpBuffer, sizeof(tmpBuffer), &writtenBytes, NULL);
					dprintf("%d bytes written @%p!\n", writtenBytes, inFileOffset);
				}
				else{
					dprintf("Failed to read physical memory at %p\n", PhysicalAddress);
				}
			}else{
				if (PhysicalAddress & 0x800){ //In transition
					PhysicalAddress = PhysicalAddress & 0x00007FFFFFFFF000;
					ddprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
					char tmpBuffer[4096];
					ULONG readBytes;
					ReadPhysical(PhysicalAddress, tmpBuffer, sizeof(tmpBuffer), &readBytes);
					if (readBytes == sizeof(tmpBuffer)){
						LONG inFileOffset = (StartingSector * 0x200) + (i * 0x1000/*TODO: PAGE_SIZE*/);
						SetFilePointer(hOutputFile, inFileOffset, NULL, FILE_BEGIN);
						ULONG writtenBytes;
						WriteFile(hOutputFile, tmpBuffer, sizeof(tmpBuffer), &writtenBytes, NULL);
						dprintf("%d bytes written @%p!\n", writtenBytes, inFileOffset);
					}
					else{
						dprintf("Failed to read physical memory at %p\n", PhysicalAddress);
					}
				}
			}
		}
		rc = ReadPointer(Subsection + g_NextSubsectionOffset, &Subsection);
		if (rc != 1){
			return false;
		}
	}
	return true;
}


DECLARE_API(dumpfiles)
{
	ULONG_PTR FileObject = 0;
	ULONG64 Value;
	PCSTR Remainder;
	int rc;

	//ULONG Bytes;
	ULONG Data = 0;
	HANDLE hOutputFile = NULL;

	BOOL ret = GetExpressionEx(args, &Value, &Remainder);
	if (ret == TRUE && Value != 0) {
		FileObject = Value;
		if (FileObject == NULL){
			dprintf("Invalid FileObject\n");
			goto Fail;
		}
	}
	else{
		dprintf("usage...\n");
		goto Fail;
	}

	LPCWSTR outputDirectory = TEXT("C:\\output\\");
	if (DirectoryExists(outputDirectory) == FALSE){
		dprintf("Output directory (%ws) doesn't exists !\n", outputDirectory);
		goto Fail;
	}

	WCHAR fullPath[1024]; //TODO: malloc !
	wcscpy_s(fullPath, outputDirectory);


	ddprintf("File Object : %p \n", FileObject);


	//Start !

	//TODO: test if is a _FILE_OBJECT

	//Get FileName
	UINT16 FileNameLength;
	rc = ReadMemory(FileObject + g_FileNameOffset, &FileNameLength, sizeof(FileNameLength), NULL);
	ddprintf("FileNameLength %d %p\n", rc, FileNameLength);
	if (rc != 1){
		goto Fail;
	}

	ULONG_PTR FileNamePtr;
	rc = ReadPointer(FileObject + g_FileNameOffset + 8 /*TODO*/, &FileNamePtr);
	ddprintf("FileNamePtr %d %p\n", rc, FileNamePtr);
	if (rc != 1 || FileNamePtr == NULL){
		goto Fail;
	}

	WCHAR FileName[1024];
	rc = ReadMemory(FileNamePtr, FileName, FileNameLength, NULL);
	FileName[FileNameLength / 2] = 0;
	ddprintf("FileName %d %ws\n", rc, FileName);
	if (rc != 1){
		goto Fail;
	}

	//Concat outputDirectory with FileName;
	wcscat_s(fullPath, FileName);
	ddprintf("fullPath %ws\n", fullPath);

	//Get Basepath
	WCHAR fullBasePath[1024]; //TODO: malloc !
	wcscpy_s(fullBasePath, fullPath);
	if (PathRemoveFileSpec(fullBasePath) == FALSE){
		dprintf("Fail to get local file base path !\n");
		goto Fail;
	}
	//Add trailling "\\"
	size_t fullBasePathLen = wcslen(fullBasePath);
	fullBasePath[fullBasePathLen] = '\\';
	fullBasePath[fullBasePathLen + 1] = '\0';
	ddprintf("fullBasePath %ws\n", fullBasePath);

	//Create directory tree
	if (DirectoryCreate(fullBasePath) == FALSE){
		dprintf("Fail to create local directory base path %ws!\n", fullBasePath);
		goto Fail;
	}

	//Creation of local file
	hOutputFile = CreateFile(fullPath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hOutputFile == INVALID_HANDLE_VALUE){
		dprintf("Failed to create local file %ws!\n", fullPath);
		goto Fail;
	}



	//Get _SECTION
	ULONG_PTR SectionObjectPointer;
	rc = ReadPointer(FileObject + g_SectionObjectPointerOffset, &SectionObjectPointer);
	ddprintf("SectionObjectPointer %d %p\n", rc, SectionObjectPointer);
	if (rc != 1 || SectionObjectPointer == NULL){
		goto Fail;
	}

	dprintf("################DataSectionObject###############\n");
	ULONG_PTR DataSectionObject;
	rc = ReadPointer(SectionObjectPointer + g_DataSectionObjectOffset, &DataSectionObject);
	ddprintf("DataSectionObject %d %p\n", rc, DataSectionObject);
	if (rc != 1){
		goto Fail;
	}

	if (DataSectionObject > 0){
		ULONG_PTR Subsection = DataSectionObject + g_SubsectionOffset;
		if (dumpSubSection(hOutputFile, Subsection) == false){
			goto Fail;
		}
	}
	else{
		dprintf("Nothing to extract from DataSectionObject \n");
	}

	dprintf("################ImageSectionObject###############\n");
	ULONG_PTR ImageSectionObject;
	rc = ReadPointer(SectionObjectPointer + g_ImageSectionObjectOffset, &ImageSectionObject);
	ddprintf("ImageSectionObject %d %p\n", rc, ImageSectionObject);
	if (rc != 1){
		goto Fail;
	}

	if (ImageSectionObject > 0){
		ULONG_PTR Subsection = ImageSectionObject + g_SubsectionOffset;
		if (dumpSubSection(hOutputFile, Subsection) == false){
			goto Fail;
		}
	}else{
		dprintf("Nothing to extract from ImageSectionObject \n");
	}

	dprintf("################SharedCacheMap###################\n");
	//_SHARED_CACHE_MAP
	ULONG_PTR SharedCacheMap;
	rc = ReadPointer(SectionObjectPointer + g_SharedCacheMapOffset, &SharedCacheMap);
	ddprintf("SharedCacheMap %d %p\n", rc, SharedCacheMap);
	if (rc != 1){
		goto Fail;
	}

	if (SharedCacheMap != NULL){
		ULONG_PTR InitialVacbs[4];
		for (int i = 0; i<4; i++){
			rc = ReadPointer(SharedCacheMap + g_InitialVacbsOffset[i], &InitialVacbs[i]);
			ddprintf("InitialVacbs[%d] %d %p\n", i, rc, InitialVacbs[i]);
			if (rc != 1){
				goto Fail;
			}

			if (InitialVacbs[i]){
				ULONG_PTR BaseAddress;
				rc = ReadPointer(InitialVacbs[i], &BaseAddress);
				ddprintf("\t==> BaseAddress %d %p\n", rc, BaseAddress);
				if (rc != 1){
					goto Fail;
				}
				char tmpBuffer[256*1024];
				ULONG readBytes;
				ReadMemory(BaseAddress, tmpBuffer, sizeof(tmpBuffer), &readBytes);
				if (readBytes == sizeof(tmpBuffer)){
					LONG inFileOffset = i * 256 * 1024;
					SetFilePointer(hOutputFile, inFileOffset, NULL, FILE_BEGIN);
					ULONG writtenBytes;
					WriteFile(hOutputFile, tmpBuffer, sizeof(tmpBuffer), &writtenBytes, NULL);
					dprintf("%d bytes written @%p!\n", writtenBytes, inFileOffset);
				}
				else{
					dprintf("Failed to read physical memory at %p\n", BaseAddress);
				}
			}
		}
		//TOOD: VACB Array !
		dprintf("TODO: VACB Array !\n");
	}else{
		dprintf("Nothing to extract from SharedCacheMap \n");
	}

	//Truncate hOutputFile to SizeOfSegment
	//SetFilePointer(hOutputFile, SizeOfSegment, NULL, FILE_BEGIN);
	//SetEndOfFile(hOutputFile);
Fail:
	if (hOutputFile != NULL){
		dprintf("File saved at %ws\n", fullPath);
		CloseHandle(hOutputFile);
	}
}

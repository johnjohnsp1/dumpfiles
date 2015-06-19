 
// cacheExt.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <cstdlib>
#include <cstdio>

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <string>

// Fix the incompatibility between Windows sal.h and g++ compiler.
/*#define _Field_size_opt_(x)
#define _Out_writes_bytes_to_(x, y)
#define _In_reads_bytes_(x)
#define _Out_writes_bytes_(x)
#define _Inout_
#define _Out_writes_to_opt_(x, y)
#define _Out_writes_to_(x, y)*/

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



DECLARE_API(help)
{
	dprintf("PIMP MY HELP\n");
}

BOOL DirectoryExists(LPCWSTR szPath){
	DWORD dwAttrib =  GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL DirectoryCreate(wchar_t *path){
	wchar_t folder[MAX_PATH];
	wchar_t *end;
	ZeroMemory(folder, MAX_PATH * sizeof(wchar_t));

	end = wcschr(path, L'\\');

	while(end != NULL){
		wcsncpy_s(folder, path, end - path + 1);
		if(!CreateDirectory(folder, NULL)){
			DWORD err = GetLastError();

			if(err != ERROR_ALREADY_EXISTS){
				// do whatever handling you'd like
			}
		}
		end = wcschr(++end, L'\\');
	}
	return TRUE;
}


DECLARE_API(dump)
{
	ULONG_PTR FileObject = 0;
	ULONG64 Value;
	PCSTR Remainder;

	//ULONG Bytes;
	ULONG Data = 0;
	HANDLE hOuputFile = NULL;

	BOOL ret = GetExpressionEx(args, &Value, &Remainder);
	if (ret == TRUE && Value != 0) {
		FileObject = Value;
		if(FileObject == NULL){
			dprintf("invalid...\n");
			goto Fail;
		}
	}else{
		dprintf("usage...\n");
		goto Fail;
	}

	LPCWSTR outputDirectory = TEXT("C:\\output\\");
	if(DirectoryExists(outputDirectory) == FALSE){
		dprintf("Output directory (%ws) doesn't exists !\n", outputDirectory);
		goto Fail;
	}

	WCHAR fullPath[1024]; //TODO: malloc !
	wcscpy_s(fullPath, outputDirectory);
	//wcscat_s(fullPath, L"DUSDLSLHLMDHLKDH");

	//return;
	dprintf("File Object : %p \n", FileObject);

	ULONG SectionObjectPointerOffset;
	int rc = GetFieldOffset("_FILE_OBJECT", "SectionObjectPointer", &SectionObjectPointerOffset);
	dprintf("SectionObjectPointerOffset %d %p\n", rc, SectionObjectPointerOffset);
	if (rc != 0){
		goto Fail;
	}

	ULONG FileNameOffset;
	rc = GetFieldOffset("_FILE_OBJECT", "FileName", &FileNameOffset);
	dprintf("FileNameOffset %d %p\n", rc, FileNameOffset);
	if (rc != 0){
		goto Fail;
	}

	ULONG DataSectionObjectOffset;
	rc = GetFieldOffset("_SECTION_OBJECT_POINTERS", "DataSectionObject", &DataSectionObjectOffset);
	dprintf("DataSectionObjectOffset %d %p\n", rc, DataSectionObjectOffset);
	if (rc != 0){
		goto Fail;
	}

	ULONG SubsectionOffset;
	SubsectionOffset = GetTypeSize("nt!_CONTROL_AREA");
	dprintf("SubsectionOffset %p\n", SubsectionOffset);
	if (rc != 0){
		goto Fail;
	}

	ULONG SubsectionBaseOffset;
	rc = GetFieldOffset("_SUBSECTION", "SubsectionBase", &SubsectionBaseOffset);
	dprintf("SubsectionBaseOffset %d %p\n", rc, SubsectionBaseOffset);
	if (rc != 0){
		goto Fail;
	}

	ULONG NumberOfPfnReferencesOffset;
	rc = GetFieldOffset("_CONTROL_AREA", "NumberOfPfnReferences", &NumberOfPfnReferencesOffset);
	dprintf("NumberOfPfnReferencesOffset %d %p\n", rc, NumberOfPfnReferencesOffset);
	if (rc != 0){
		goto Fail;
	}

	ULONG SegmentOffset;
	rc = GetFieldOffset("_CONTROL_AREA", "Segment", &SegmentOffset);
	dprintf("SegmentOffset %d %p\n", rc, SegmentOffset);
	if (rc != 0){
		goto Fail;
	}
	
	
	ULONG PtesInSubsectionOffset;
	rc = GetFieldOffset("_SUBSECTION", "PtesInSubsection", &PtesInSubsectionOffset);
	dprintf("PtesInSubsectionOffset %d %p\n", rc, PtesInSubsectionOffset);
	if (rc != 0){
		goto Fail;
	}

	ULONG NextSubsectionOffset;
	rc = GetFieldOffset("_SUBSECTION", "NextSubsection", &NextSubsectionOffset);
	dprintf("NextSubsectionOffset %d %p\n", rc, NextSubsectionOffset);
	if (rc != 0){
		goto Fail;
	}

	//Start !
	//Get FileName
	UINT16 FileNameLength;
	rc = ReadMemory(FileObject + FileNameOffset, &FileNameLength, sizeof(FileNameLength), NULL);
	dprintf("FileNameLength %d %p\n", rc, FileNameLength);
	if (rc != 1){
		goto Fail;
	}

	ULONG_PTR FileNamePtr;
	rc = ReadPointer(FileObject + FileNameOffset + 8 /*TODO*/, &FileNamePtr);
	dprintf("FileNamePtr %d %p\n", rc, FileNamePtr);
	if (rc != 1){
		goto Fail;
	}

	WCHAR FileName[1024];
	rc = ReadMemory(FileNamePtr, FileName, FileNameLength, NULL);
	FileName[FileNameLength/2] = 0;
	dprintf("FileName %d %ws\n", rc, FileName);
	if (rc != 1){
		goto Fail;
	}

	//Concat outputDirectory with FileName;
	wcscat_s(fullPath, FileName);
	dprintf("fullPath %ws\n", fullPath);

	//Get Basepath
	WCHAR fullBasePath[1024]; //TODO: malloc !
	wcscpy_s(fullBasePath, fullPath);
	if(PathRemoveFileSpec(fullBasePath) == FALSE){
		dprintf("Fail to get local file base path !\n");
		goto Fail;
	}
	//Add trailling "\\"
	size_t fullBasePathLen = wcslen(fullBasePath);
	fullBasePath[fullBasePathLen] = '\\';
	fullBasePath[fullBasePathLen+1] = '\0';
	dprintf("fullBasePath %ws\n", fullBasePath);

	//Create directory tree
	if(DirectoryCreate(fullBasePath) == FALSE){
		dprintf("Fail to create local directory base path !\n");
		goto Fail;
	}

	//Creation of local file
	hOuputFile = CreateFile(fullPath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if(hOuputFile ==  INVALID_HANDLE_VALUE){
		dprintf("Failed to create local file !\n");
		goto Fail;
	}



	//Get _SECTION
	ULONG_PTR SectionObjectPointer;
	rc = ReadPointer(FileObject + SectionObjectPointerOffset, &SectionObjectPointer);
	dprintf("SectionObjectPointer %d %p\n", rc, SectionObjectPointer);
	if (rc != 1){
		goto Fail;
	}

	ULONG_PTR DataSectionObject;
	rc = ReadPointer(SectionObjectPointer + DataSectionObjectOffset, &DataSectionObject);
	dprintf("DataSectionObject %d %p\n", rc, DataSectionObject);
	if (rc != 1){
		goto Fail;
	}

	//Get _SEGMENT
	ULONG_PTR Segment;
	rc = ReadPointer(DataSectionObject + SegmentOffset, &Segment);
	dprintf("Segment %d %p\n", rc, Segment);
	if (rc != 1){
		goto Fail;
	}

	//Get SizeOfSegment
	UINT16 SizeOfSegment;
	rc = ReadMemory(Segment + 0x18 /*TODO*/, &SizeOfSegment, sizeof(SizeOfSegment), NULL);
	dprintf("SizeOfSegment %d %d\n", rc, SizeOfSegment);
	if (rc != 1){
		goto Fail;
	}


	ULONG_PTR Subsection = DataSectionObject + SubsectionOffset;
	while(Subsection > 0){
		dprintf("Subsection %p\n", Subsection);

		UINT32 NumberOfPfnReferences;
		rc = ReadMemory(DataSectionObject + NumberOfPfnReferencesOffset, &NumberOfPfnReferences, sizeof(NumberOfPfnReferences), NULL);
		dprintf("NumberOfPfnReferences %d %p\n", rc, NumberOfPfnReferences);
		if (rc != 1){
			goto Fail;
		}
	
		UINT32 PtesInSubsection;
		rc = ReadMemory(Subsection + PtesInSubsectionOffset, &PtesInSubsection, sizeof(PtesInSubsection), NULL);
		dprintf("PtesInSubsection %d %p\n", rc, PtesInSubsection);
		if (rc != 1){
			goto Fail;
		}
		//#define MIN(a, b) (((a)<(b))?(a):(b));
		//UINT32 RealPteCount = MIN(PtesInSubsection, NumberOfPfnReferences);
		//dprintf("RealPteCount %d\n", RealPteCount);
		//if(PtesInSubsection > 0){
		ULONG_PTR SubsectionBase;
		rc = ReadPointer(Subsection + SubsectionBaseOffset, &SubsectionBase);
		dprintf("SubsectionBase %d %p\n", rc, SubsectionBase);
		if (rc != 1){
			goto Fail;
		}

		for(UINT32 i=0; i<PtesInSubsection; i++){
			ULONG_PTR PhysicalAddress;
			rc = ReadMemory(SubsectionBase + i*sizeof(ULONG_PTR), &PhysicalAddress, sizeof(PhysicalAddress), NULL);
			if (rc != 1){
				goto Fail;
			}
			//dprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
			if(PhysicalAddress & 0x1){ //Is valid
				//TODO !
				//PhysicalAddress = PhysicalAddress & 0xFFFFFFFFFFFFF000;
				//dprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
			}else{
				if(PhysicalAddress & 0x800){ //In transition
					PhysicalAddress = PhysicalAddress & 0xFFFFFFFFFFFFF000;
					dprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
					char tmpBuffer[4096];
					ULONG readBytes;
					ReadPhysical(PhysicalAddress, tmpBuffer, sizeof(tmpBuffer), &readBytes);
					if(readBytes == sizeof(tmpBuffer)){
						ULONG writtenBytes;
						WriteFile(hOuputFile, tmpBuffer, sizeof(tmpBuffer), &writtenBytes, NULL);
						dprintf("Write done !\n");
					}else{
						dprintf("Failed to read physical memory at %p\n", PhysicalAddress);
					}
				}
			}
		}
		//}else{
		//	dprintf("No Pte in subsection !\n");
		//}
		rc = ReadPointer(Subsection + NextSubsectionOffset, &Subsection);
		if (rc != 1){
			goto Fail;
		}
	}

	//_SHARED_CACHE_MAP
	/*ULONG SharedCacheMapOffset;
	rc = GetFieldOffset("_SECTION_OBJECT_POINTERS", "SharedCacheMap", &SharedCacheMapOffset);
	dprintf("%d %p\n", rc, SharedCacheMapOffset);
	if (rc != 0){
		return;
	}

	ULONG_PTR SharedCacheMap;
	rc = ReadPointer(SectionObjectPointer + SharedCacheMapOffset, &SharedCacheMap);
	dprintf("SharedCacheMap %d %p\n", rc, SharedCacheMap);
	if (rc != 1){
		return;
	}

	if(SharedCacheMap != NULL){
		dprintf("Got it!\n");
		ULONG InitialVacbsOffset[4];
		rc = GetFieldOffset("_SHARED_CACHE_MAP", "InitialVacbs[0]", &InitialVacbsOffset[0]);
		rc = GetFieldOffset("_SHARED_CACHE_MAP", "InitialVacbs[1]", &InitialVacbsOffset[1]);
		rc = GetFieldOffset("_SHARED_CACHE_MAP", "InitialVacbs[2]", &InitialVacbsOffset[2]);
		rc = GetFieldOffset("_SHARED_CACHE_MAP", "InitialVacbs[3]", &InitialVacbsOffset[3]);
		dprintf("%d %p\n", rc, InitialVacbsOffset[3]);
		if (rc != 0){
			return;
		}
	
		ULONG_PTR InitialVacbs[4];
		for(int i=0; i<4; i++){
			rc = ReadPointer(SharedCacheMap + InitialVacbsOffset[i], &InitialVacbs[i]);
			dprintf("InitialVacbs[%d] %d %p\n", i, rc, InitialVacbs[i]);
			if (rc != 1){
				return;
			}

			if(InitialVacbs[i]){
				ULONG_PTR BaseAddress;
				rc = ReadPointer(InitialVacbs[i], &BaseAddress);
				dprintf("\t==> BaseAddress %d %p\n", rc, BaseAddress);
				if (rc != 1){
					return;
				}


			}
		}
	}*/

	//Truncate hOuputFile to SizeOfSegment
	SetFilePointer(hOuputFile, SizeOfSegment, NULL, FILE_BEGIN);
	SetEndOfFile(hOuputFile);

Fail:
	if(hOuputFile != NULL){
		CloseHandle(hOuputFile);
	}
}


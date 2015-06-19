 
// cacheExt.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <cstdlib>
#include <cstdio>

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>


#define _Field_size_opt_(x)
#define _Out_writes_bytes_to_(x, y)
#define _In_reads_bytes_(x)
#define _Out_writes_bytes_(x)
#define _Inout_
#define _Out_writes_to_opt_(x, y)
#define _Out_writes_to_(x, y)

#define KDEXT_64BIT
#include <wdbgexts.h>


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
	return &ApiVersion;
}

extern "C"
VOID
__declspec(dllexport)
CheckVersion(
VOID
) {
	return;
}



//Windbg commands !

DECLARE_API(help)
{
	dprintf("PIMP MY HELP\n");
}




DECLARE_API(dump)
{
	ULONG_PTR FileObject = 0;
	ULONG64 Value;
	PCSTR Remainder;

	ULONG Bytes;
	ULONG Data = 0;

	BOOL ret = GetExpressionEx(args, &Value, &Remainder);

	if (ret == TRUE && Value != 0) {
		FileObject = Value;
		if(FileObject == NULL){
			dprintf("invalid...\n");
			return;
		}
	}else{
		dprintf("usage...\n");
		return;
	}

	dprintf("File Object : %p \n", FileObject);

	ULONG SectionObjectPointerOffset;
	int rc = GetFieldOffset("_FILE_OBJECT", "SectionObjectPointer", &SectionObjectPointerOffset);
	dprintf("SectionObjectPointerOffset %d %p\n", rc, SectionObjectPointerOffset);
	if (rc != 0){
		return;
	}

	ULONG DataSectionObjectOffset;
	rc = GetFieldOffset("_SECTION_OBJECT_POINTERS", "DataSectionObject", &DataSectionObjectOffset);
	dprintf("DataSectionObjectOffset %d %p\n", rc, DataSectionObjectOffset);
	if (rc != 0){
		return;
	}

	ULONG SubsectionOffset;
	SubsectionOffset = GetTypeSize("nt!_CONTROL_AREA");
	dprintf("SubsectionOffset %p\n", SubsectionOffset);
	if (rc != 0){
		return;
	}

	ULONG SubsectionBaseOffset;
	rc = GetFieldOffset("_SUBSECTION", "SubsectionBase", &SubsectionBaseOffset);
	dprintf("SubsectionBaseOffset %d %p\n", rc, SubsectionBaseOffset);
	if (rc != 0){
		return;
	}

	ULONG NumberOfPfnReferencesOffset;
	rc = GetFieldOffset("_CONTROL_AREA", "NumberOfPfnReferences", &NumberOfPfnReferencesOffset);
	dprintf("NumberOfPfnReferencesOffset %d %p\n", rc, NumberOfPfnReferencesOffset);
	if (rc != 0){
		return;
	}
	
	ULONG PtesInSubsectionOffset;
	rc = GetFieldOffset("_SUBSECTION", "PtesInSubsection", &PtesInSubsectionOffset);
	dprintf("PtesInSubsectionOffset %d %p\n", rc, PtesInSubsectionOffset);
	if (rc != 0){
		return;
	}

	ULONG NextSubsectionOffset;
	rc = GetFieldOffset("_SUBSECTION", "NextSubsection", &NextSubsectionOffset);
	dprintf("NextSubsectionOffset %d %p\n", rc, NextSubsectionOffset);
	if (rc != 0){
		return;
	}

	//Start !

	ULONG_PTR SectionObjectPointer;
	rc = ReadPointer(FileObject + SectionObjectPointerOffset, &SectionObjectPointer);
	dprintf("SectionObjectPointer %d %p\n", rc, SectionObjectPointer);
	if (rc != 1){
		return;
	}

	ULONG_PTR DataSectionObject;
	rc = ReadPointer(SectionObjectPointer + DataSectionObjectOffset, &DataSectionObject);
	dprintf("DataSectionObject %d %p\n", rc, DataSectionObject);
	if (rc != 1){
		return;
	}

	ULONG_PTR Subsection = DataSectionObject + SubsectionOffset;
	while(Subsection > 0){
		dprintf("Subsection %p\n", Subsection);

		UINT32 NumberOfPfnReferences;
		rc = ReadMemory(DataSectionObject + NumberOfPfnReferencesOffset, &NumberOfPfnReferences, sizeof(NumberOfPfnReferences), NULL);
		dprintf("NumberOfPfnReferences %d %p\n", rc, NumberOfPfnReferences);
		if (rc != 1){
			return;
		}
	
		UINT32 PtesInSubsection;
		rc = ReadMemory(Subsection + PtesInSubsectionOffset, &PtesInSubsection, sizeof(PtesInSubsection), NULL);
		dprintf("PtesInSubsection %d %p\n", rc, PtesInSubsection);
		if (rc != 1){
			return;
		}
		//#define MIN(a, b) (((a)<(b))?(a):(b));
		//UINT32 RealPteCount = MIN(PtesInSubsection, NumberOfPfnReferences);
		//dprintf("RealPteCount %d\n", RealPteCount);
		//if(PtesInSubsection > 0){
		ULONG_PTR SubsectionBase;
		rc = ReadPointer(Subsection + SubsectionBaseOffset, &SubsectionBase);
		dprintf("SubsectionBase %d %p\n", rc, SubsectionBase);
		if (rc != 1){
			return;
		}

		for(UINT32 i=0; i<PtesInSubsection; i++){
			ULONG_PTR PhysicalAddress;
			rc = ReadMemory(SubsectionBase + i*sizeof(ULONG_PTR), &PhysicalAddress, sizeof(PhysicalAddress), NULL);
			if (rc != 1){
				return;
			}
			dprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
			if(PhysicalAddress & 0x1){ //Is valid
				//PhysicalAddress = PhysicalAddress & 0xFFFFFFFFFFFFF000;
				//dprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
			}else{
				if(PhysicalAddress & 0x800){ //In transition
					PhysicalAddress = PhysicalAddress & 0xFFFFFFFFFFFFF000;
					dprintf("\t==> PhysicalAddress[%d] %d %p\n", i, rc, PhysicalAddress);
				}
			}
		}
		//}else{
		//	dprintf("No Pte in subsection !\n");
		//}
		Subsection = ReadPointer(Subsection + NextSubsectionOffset, &Subsection);
	}

	//_SHARED_CACHE_MAP
	ULONG SharedCacheMapOffset;
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
	}
}


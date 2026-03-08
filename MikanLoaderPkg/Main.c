#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/BaseMemoryLib.h>
#include  <Protocol/LoadedImage.h>
//#include  <Protocol/SimpleFileSystem.h>
//#include  <Protocol/DiskIo2.h>
//#include  <Protocol/BlockIo.h>
#include  <Guid/FileInfo.h>

typedef UINT64 Elf64_Addr;
typedef UINT64 Elf64_Off;
typedef UINT16 Elf64_Half;
typedef UINT32 Elf64_Word;
typedef UINT64 Elf64_Xword;

#define PT_LOAD 1

typedef struct {
  unsigned char e_ident[16];
  Elf64_Half    e_type;
  Elf64_Half    e_machine;
  Elf64_Word    e_version;
  Elf64_Addr    e_entry;
  Elf64_Off     e_phoff;
  Elf64_Off     e_shoff;
  Elf64_Word    e_flags;
  Elf64_Half    e_ehsize;
  Elf64_Half    e_phentsize;
  Elf64_Half    e_phnum;
  Elf64_Half    e_shentsize;
  Elf64_Half    e_shnum;
  Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

typedef struct {
  Elf64_Word  p_type;
  Elf64_Word  p_flags;
  Elf64_Off   p_offset;
  Elf64_Addr  p_vaddr;
  Elf64_Addr  p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;

struct MemoryMap {
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  return gBS->GetMemoryMap(
    &map->map_size,
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,
    &map->map_key,
    &map->descriptor_size,
    &map->descriptor_version);
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}

EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
  CHAR8 buf[256];
  UINTN len;

  CHAR8 *header =
      "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
        map->buffer, map->map_size);

  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++)
  {
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
    len = AsciiSPrint(
        buf, sizeof(buf),
        "%u, %x, %-ls, %08lx, %lx, %lx\n",
        i, desc->Type, GetMemoryTypeUnicode(desc->Type),
        desc->PhysicalStart, desc->NumberOfPages,
        desc->Attribute & 0xffffflu);
    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

  gBS->OpenProtocol(
      image_handle,
      &gEfiLoadedImageProtocolGuid,
      (VOID **)&loaded_image,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(
      loaded_image->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID **)&fs,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL** gop) {
  EFI_STATUS status;
  UINTN num_gop_handles = 0;
  EFI_HANDLE* gop_handles = NULL;

  status = gBS->LocateHandleBuffer(
      ByProtocol,
      &gEfiGraphicsOutputProtocolGuid,
      NULL,
      &num_gop_handles,
      &gop_handles);
  if (EFI_ERROR(status)) {
    return status;
  }

  status = gBS->OpenProtocol(
      gop_handles[0],
      &gEfiGraphicsOutputProtocolGuid,
      (VOID**)gop,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (EFI_ERROR(status)) {
    return status;
  }

  FreePool(gop_handles);

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table) {
  EFI_STATUS status;

  Print(L"Hello, Mikan World!\n");
  
  CHAR8 memmap_buf[4096 * 4];
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
  GetMemoryMap(&memmap);

  EFI_FILE_PROTOCOL* root_dir;
  OpenRootDir(image_handle, &root_dir);

  EFI_FILE_PROTOCOL* memmap_file;
  root_dir->Open(
    root_dir, &memmap_file, L"\\memmap",
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);

  EFI_FILE_PROTOCOL *kernel_file;
  status = root_dir->Open(
      root_dir, &kernel_file, L"\\kernel.elf",
      EFI_FILE_MODE_READ, 0);
  Print(L"open kernel: %r\n", status);

  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
  UINT8 file_info_buffer[file_info_size];
  status = kernel_file->GetInfo(
      kernel_file, &gEfiFileInfoGuid,
      &file_info_size, file_info_buffer);
  Print(L"open kernel info: %r\n", status);

  EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
  UINTN kernel_file_size = file_info->FileSize;

  // ELFファイルを一時領域(0x100000)にまるごとロード
  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
  gBS->AllocatePages(
    AllocateAddress, EfiLoaderData,
    (kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
  status = kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
  Print(L"Kernel: 0x%0lx (%lu bytes): %r\n", kernel_base_addr, kernel_file_size, status);

  // ELFプログラムヘッダを解析してPT_LOADセグメントをVMAに配置
  Elf64_Ehdr *ehdr = (Elf64_Ehdr*)kernel_base_addr;
  UINT64 kernel_first_addr = MAX_UINT64, kernel_last_addr = 0;
  for (UINTN i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = (Elf64_Phdr*)(kernel_base_addr + ehdr->e_phoff + ehdr->e_phentsize * i);
    if (phdr->p_type != PT_LOAD) continue;
    if (phdr->p_vaddr < kernel_first_addr) kernel_first_addr = phdr->p_vaddr;
    if (phdr->p_vaddr + phdr->p_memsz > kernel_last_addr) kernel_last_addr = phdr->p_vaddr + phdr->p_memsz;
  }
  EFI_PHYSICAL_ADDRESS kernel_load_addr = kernel_first_addr;
  gBS->AllocatePages(
    AllocateAddress, EfiLoaderData,
    (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000, &kernel_load_addr);
  for (UINTN i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = (Elf64_Phdr*)(kernel_base_addr + ehdr->e_phoff + ehdr->e_phentsize * i);
    if (phdr->p_type != PT_LOAD) continue;
    CopyMem((VOID*)phdr->p_vaddr, (VOID*)(kernel_base_addr + phdr->p_offset), phdr->p_filesz);
    SetMem((VOID*)(phdr->p_vaddr + phdr->p_filesz), phdr->p_memsz - phdr->p_filesz, 0);
  }

  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
  OpenGOP(image_handle, &gop);

  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  if (EFI_ERROR(status)) {
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
      Print(L"failed to get memory map: %r\n", status);
      while (1);
    }
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
      Print(L"Could not exit boot service: %r\n", status);
      while (1);
    }
  }

  UINT64 entry_addr = ehdr->e_entry;
  typedef void EntryPointType(UINT64, UINT64);
  EntryPointType* entry_point = (EntryPointType*)entry_addr;
  entry_point(gop->Mode->FrameBufferBase, gop->Mode->FrameBufferSize);

  Print(L"ALL DONE\n");

  while (1);
  return EFI_SUCCESS;
}

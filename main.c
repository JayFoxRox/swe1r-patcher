/*

  Star Wars Episode 1: Racer - Patcher
  
  (c)2018 Jannik Vogel

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>

static off_t mapExe(uint32_t offset) {
  /*
    From `objdump -x swep1rcr.exe` for the patched US version:

    0 .text         000aa750  00401000  00401000  00000400  2**2
                    CONTENTS, ALLOC, LOAD, READONLY, CODE
    1 .rdata        000054a2  004ac000  004ac000  000aac00  2**2
                    CONTENTS, ALLOC, LOAD, READONLY, DATA
    2 .data         00023600  004b2000  004b2000  000b0200  2**2
                    CONTENTS, ALLOC, LOAD, DATA
    3 .rsrc         000017b8  00ece000  00ece000  000d3800  2**2
  */

  if (offset >= 0x00ed0000) {
    // hack
    offset -= 0x00ed0000;
    offset += 0x000d5000;
  } else if (offset >= 0x00ece000) {
    // .rsrc
    offset -= 0x00ece000;
    offset += 0x000d3800;
  } else if (offset >= 0x004b2000) {
   // .data
    offset -= 0x004b2000;
    offset += 0x000b0200;
  } else if (offset >= 0x004ac000) {
    // .rdata
    offset -= 0x004ac000;
    offset += 0x000aac00;
  } else if (offset >= 0x00401000) {
    // .text
    offset -= 0x00401000;
    offset += 0x00000400;
  } else {
    printf("Unknown offset: 0x%08X\n", offset);
  }
  return offset;
}

static void* readExe(FILE* f, uint32_t offset, size_t size) {

  // Find correct location in binary
  offset = mapExe(offset);

  // Allocate space for the read
  void* data = malloc(size);

  // Read the data from the exe
  off_t previous_offset = ftell(f);
  fseek(f, offset, SEEK_SET);
  fread(data, size, 1, f);
  fseek(f, previous_offset, SEEK_SET);

  return data;
}

static void dumpTexture(FILE* f, size_t offset, uint8_t unk0, uint8_t unk1, unsigned int width, unsigned int height, const char* filename) {
  // Presumably the format information?
  assert(unk0 == 3);
  assert(unk1 == 0);

  FILE* out = fopen(filename, "wb");
  fprintf(out, "P3\n%d %d\n15\n", width, height);

  // Copy the pixel data
  uint8_t* texture = readExe(f, offset, width * height * 4 / 8);
  for(unsigned int i = 0; i < width * height * 2; i++) {
    uint8_t v = ((texture[i / 2] << ((i % 2) * 4)) & 0xF0) >> 4;
    fprintf(out, "%d %d %d\n", v, v, v);
  }
  free(texture);

  fclose(out);
  return;
}

static uint32_t dumpTextureTable(FILE* f, uint32_t offset, uint8_t unk0, uint8_t unk1, unsigned int width, unsigned int height, const char* filename) {
  char filename_i[4096]; //FIXME: Size

  // Get size of the table
  uint32_t* buffer = readExe(f, offset + 0, 4);
  uint32_t count = *buffer;
  free(buffer);

  // Loop over elements and dump each
  printf("// %s at 0x%X\n", filename, offset);
  uint32_t* offsets = readExe(f, offset + 4, count * 4);
  for(unsigned int i = 0; i < count; i++) {
    sprintf(filename_i, "%s_%d.ppm", filename, i);
    printf("// - [%d]: 0x%X\n", i, offsets[i]);
    dumpTexture(f, offsets[i], unk0, unk1, width, height, filename_i);
  }
  free(offsets);
  return count;
}

static char* escapeString(const char* string) {
  char* escaped_string = malloc(strlen(string) * 2 + 1);
  const char* s = string;
  char* d = escaped_string;
  while(*s != '\0') {
    if ((*s == '\"') || (*s == '\'') || (*s == '\\')) {
      *d++ = '\\';
    }
    *d++ = *s++;
  }
  *d = '\0';
  return escaped_string;
}

static char* reallocEscapedString(char* string) {
  char* escaped_string = escapeString(string);
  free(string);
  return escaped_string;
}

static uint32_t patchTextureTable(FILE* f, uint32_t memory_offset, uint32_t offset, uint32_t code_begin_offset, uint32_t code_end_offset, uint32_t width, uint32_t height, const char* filename) {

  // Some helpful constants and helper variable for x86
  const uint8_t nop = 0x90;
  const uint8_t push_i32 = 0x68;
  const uint8_t jmp = 0xE9;
  uint32_t jmp_target;

  // Create a code cave
  // The original argument for the width is only 8 bit (signed), so it's hard
  // to extend. That's why we use a code cave.
  uint32_t cave_memory_offset = memory_offset;
  fseek(f, mapExe(cave_memory_offset), SEEK_SET);
  uint32_t cave_file_offset = ftell(f);

#if 1
  // Attempt to realign the disassembler
  for(unsigned int i = 0; i < 16; i++) {
    fwrite(&nop, 1, 1, f);
  }
  cave_memory_offset += 16;
  cave_file_offset += 16;
  memory_offset += 16;
#endif

  // Patches the arguments for the texture loader
  fwrite(&push_i32, 1, 1, f);
  fwrite(&height, 4, 1, f);
  fwrite(&push_i32, 1, 1, f);
  fwrite(&width, 4, 1, f);
  fwrite(&push_i32, 1, 1, f);
  fwrite(&height, 4, 1, f);
  fwrite(&push_i32, 1, 1, f);
  fwrite(&width, 4, 1, f);
  jmp_target = code_end_offset - (cave_memory_offset + (ftell(f) - cave_file_offset) + 5);
  fwrite(&jmp, 1, 1, f);
  fwrite(&jmp_target, 4, 1, f);

  memory_offset += ftell(f) - cave_file_offset;

  // Write code to jump into the codecave (5 bytes) and clear original code
  fseek(f, mapExe(code_begin_offset), SEEK_SET);
  jmp_target = cave_memory_offset - (code_begin_offset + 5);
  fwrite(&jmp, 1, 1, f);
  printf("Tying to jump to 0x%08X\n", cave_memory_offset);
  fwrite(&jmp_target, 4, 1, f);
  for(unsigned int i = 5; i < (code_end_offset - code_begin_offset); i++) {
    fwrite(&nop, 1, 1, f);
  }

  // Patch textures
  fseek(f, mapExe(offset), SEEK_SET);
  uint32_t count;
  fread(&count, 4, 1, f);

  // Have a buffer for pixeldata
  unsigned int texture_size = width * height * 4 / 8;
  uint8_t* buffer = malloc(texture_size);

  //FIXME: Loop over all textures
  for(unsigned int i = 0; i < count; i++) {

    printf("At 0x%X in %d / %d\n", ftell(f), i, count - 1);
    uint32_t texture_old;
    fread(&texture_old, 4, 1, f);
    fseek(f, -4, SEEK_CUR);
    uint32_t texture_new = memory_offset;
    memory_offset += texture_size;
    fwrite(&texture_new, 4, 1, f);
    printf("%d: 0x%X -> 0x%X\n", count, texture_old, texture_new);

  //FIXME: Fixup the format?
  //.text:0042D794                 push    0
  //.text:0042D796                 push    3

    char path[4096];
    sprintf(path, "%s_%d_test.data", filename, i);
    printf("Loading '%s'\n", path);
    FILE* ft = fopen(path, "rb");
    assert(ft != NULL);
    memset(buffer, 0x00, texture_size);
    for(unsigned int i = 0; i < texture_size * 2; i++) {
      uint8_t pixel[2]; // GIMP only exports Gray + Alpha..
      fread(pixel, sizeof(pixel), 1, ft);
      buffer[i / 2] |= (pixel[0] & 0xF0) >> ((i % 2) * 4);
    }
    fclose(ft);
    
    // Write pixel data, but maintain offset in file
    off_t table_offset = ftell(f);
    fseek(f, mapExe(texture_new), SEEK_SET);
    fwrite(buffer, 1, texture_size, f);
    fseek(f, table_offset, SEEK_SET);

  }
  free(buffer);

  return memory_offset;
}

static uint8_t read8(FILE* f, off_t offset) {
  uint8_t value;
  fseek(f, offset, SEEK_SET);
  fread(&value, 1, 1, f);
  return value;
}

static void write8(FILE* f, off_t offset, uint8_t value) {
  fseek(f, offset, SEEK_SET);
  fwrite(&value, 1, 1, f);
  return;
}

static uint16_t read16(FILE* f, off_t offset) {
  uint16_t value;
  fseek(f, offset, SEEK_SET);
  fread(&value, 2, 1, f);
  return value;
}

static void write16(FILE* f, off_t offset, uint16_t value) {
  fseek(f, offset, SEEK_SET);
  fwrite(&value, 2, 1, f);
  return;
}

static void patch16_add(FILE* f, off_t offset, uint16_t delta) {
  write16(f, offset, read16(f, offset) + delta);
  return;
}

static uint32_t read32(FILE* f, off_t offset) {
  uint32_t value;
  fseek(f, offset, SEEK_SET);
  fread(&value, 4, 1, f);
  return value;
}

static void write32(FILE* f, off_t offset, uint32_t value) {
  fseek(f, offset, SEEK_SET);
  fwrite(&value, 4, 1, f);
  return;
}

static void patch32_add(FILE* f, off_t offset, uint32_t delta) {
  write32(f, offset, read32(f, offset) + delta);
  return;
}

int main(int argc, char* argv[]) {

  FILE* f = fopen(argv[1], "rb+");
  assert(f != NULL);

  // Read timestamp of binary to see which base version this is
  uint32_t timestamp = read32(f, 216);

  //FIXME: Now set the correct pointers for this binary
  switch(timestamp) {
  case 0x3C60692C:
    break;
  default:
    printf("Unsupported version of the game, timestamp 0x%08X\n", timestamp);
    return 1;
  }


  // Allocate more space, say... 4MB?
  // (we use the .rsrc section, which is last in memory)
  uint32_t patch_size = 4 * 1024 * 1024;


  // Get rough offset where we'll place our stuff
  fseek(f, 0, SEEK_END);
  uint32_t file_offset = ftell(f);

  // Align offset to safe bound and create data
  fseek(f, file_offset, SEEK_SET);
  file_offset = (file_offset + 0xFFF) & ~0xFFF;
  while(ftell(f) < (file_offset + patch_size)) {
    uint8_t dummy = 0x00;
    fwrite(&dummy, 1, 1, f);
  }

  // Select a unused memory region and align it
  uint32_t memory_offset = read32(f, 288);
  memory_offset = (memory_offset + 0xFFF) & ~0xFFF;

  // Create data for new section
  uint32_t characteristics = 0;
  characteristics |= 0x20; // Code
  characteristics |= 0x40; // Initialized Data
  characteristics |= 0x20000000; // Executable
  characteristics |= 0x40000000; // Readable
  characteristics |= 0x80000000; // Writeable

  // Append a new section
  write32(f, 576 + 40 + 0, *(uint32_t*)"hack");
  write32(f, 576 + 40 + 4, 0x00000000);
  write32(f, 576 + 40 + 8, patch_size);
  write32(f, 576 + 40 + 12, memory_offset);
  write32(f, 576 + 40 + 16, patch_size);
  write32(f, 576 + 40 + 20, file_offset);
  write32(f, 576 + 40 + 24, 0x00000000);
  write32(f, 576 + 40 + 28, 0x00000000);
  write32(f, 576 + 40 + 32, 0x00000000);
  write32(f, 576 + 40 + 36, characteristics);

#if 1

  // Increment number of sections
  patch16_add(f, 214, 1);

  // size of image
  write32(f, 288, memory_offset + patch_size);

  // size of code
  patch32_add(f, 236, patch_size);

  // size of intialized data
  patch32_add(f, 240, patch_size);
#endif


  // Add image base
  memory_offset += read32(f, 260);


#if 1
  memory_offset = patchTextureTable(f, memory_offset, 0x4BF91C, 0x42D745, 0x42D753, 512, 1024, "font0");  
  memory_offset = patchTextureTable(f, memory_offset, 0x4BF7E4, 0x42D786, 0x42D794, 512, 1024, "font1");
  memory_offset = patchTextureTable(f, memory_offset, 0x4BF84C, 0x42D7C7, 0x42D7D5, 512, 1024, "font2");
  memory_offset = patchTextureTable(f, memory_offset, 0x4BF8B4, 0x42D808, 0x42D816, 512, 1024, "font3");
  //FIXME: font4
#endif


#if 0
  dumpTextureTable(f, 0x4BF91C, 3, 0, 64, 128, "font0");
  dumpTextureTable(f, 0x4BF7E4, 3, 0, 64, 128, "font1");
  dumpTextureTable(f, 0x4BF84C, 3, 0, 64, 128, "font2");
  dumpTextureTable(f, 0x4BF8B4, 3, 0, 64, 128, "font3");
  dumpTextureTable(f, 0x4BF984, 3, 0, 64, 128, "font4");
#endif

#if 1
  // Upgrade network play updates to 100%

  // Patch the game GUID so people don't cheat with it (as easily):
  fseek(f, mapExe(0x4AF9B0), SEEK_SET);
  uint32_t mark_hack = 0x1337C0DE;
  fwrite(&mark_hack, 4, 1, f);

  // Important: Must be updated every time we change something in the network!
  uint32_t version = 0x00000000;
  fwrite(&version, 4, 1, f);
  
  // Configure upgrade for menu
  uint8_t upgrade_levels[7]  = {    5,    5,    5,    5,    5,    5,    5 };
  uint8_t upgrade_healths[7] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  
  // Now do the actual upgrade for menus
  fseek(f, mapExe(0x45CFC6), SEEK_SET);
  fwrite(&upgrade_levels[0], 1, 1, f);
  fseek(f, mapExe(0x45CFCB), SEEK_SET);
  fwrite(&upgrade_healths[0], 1, 1, f);

  //FIXME: Upgrade network player creation

  // 0x45B725 vs 0x45B9FF

#if 0
lea     edx, [esp+1Ch+upgrade_health]
lea     eax, [esp+1Ch+upgrade_level]
push    edx             ; upgrade_healths
push    eax             ; upgrade_levels
push    ebp             ; handling_in
push    offset handling_out ; handling_out
mov     [esp+esi+2Ch+upgrade_health], cl
call    _sub_449D00_generate_upgraded_handling_table_data
#endif

// Place upgrade data in memory

fseek(f, mapExe(memory_offset), SEEK_SET);

uint32_t memory_offset_upgrade_levels = memory_offset;
fwrite(upgrade_levels, 7, 1, f);
memory_offset += 7;

uint32_t memory_offset_upgrade_healths = memory_offset;
fwrite(upgrade_healths, 7, 1, f);
memory_offset += 7;


// Now inject the code

uint32_t memory_offset_upgrade_code = memory_offset;

//  -> push edx
write8(f, mapExe(memory_offset), 0x52); memory_offset += 1;

//  -> push eax
write8(f, mapExe(memory_offset), 0x50); memory_offset += 1;

//  -> push offset upgrade_healths
write8(f, mapExe(memory_offset), 0x68); memory_offset += 1;
write32(f, mapExe(memory_offset), memory_offset_upgrade_healths); memory_offset += 4;

//  -> push offset upgrade_levels
write8(f, mapExe(memory_offset), 0x68); memory_offset += 1;
write32(f, mapExe(memory_offset), memory_offset_upgrade_levels); memory_offset += 4;

//  -> push esi
write8(f, mapExe(memory_offset), 0x56); memory_offset += 1;

//  -> push edi
write8(f, mapExe(memory_offset), 0x57); memory_offset += 1;

//  -> call _sub_449D00
write8(f, mapExe(memory_offset), 0xE8); memory_offset += 1;
write32(f, mapExe(memory_offset), 0x449D00 - (memory_offset + 4)); memory_offset += 4;

//  -> add esp, 0x10
write8(f, mapExe(memory_offset), 0x83); memory_offset += 1;
write8(f, mapExe(memory_offset), 0xC4); memory_offset += 1;
write8(f, mapExe(memory_offset), 0x10); memory_offset += 1;

//  -> pop eax
write8(f, mapExe(memory_offset), 0x58); memory_offset += 1;

//  -> pop edx
write8(f, mapExe(memory_offset), 0x5A); memory_offset += 1;

//  -> retn
write8(f, mapExe(memory_offset), 0xC3); memory_offset += 1;


// Install it by jumping from 0x45B765 and returning to 0x45B76C

write8(f, mapExe(0x45B765 + 0), 0xE8);
write32(f, mapExe(0x45B765 + 1), memory_offset_upgrade_code - (0x45B765 + 5));
write8(f, mapExe(0x45B765 + 5), 0x90);
write8(f, mapExe(0x45B765 + 6), 0x90);

#endif

  fclose(f);

  return 0;
}

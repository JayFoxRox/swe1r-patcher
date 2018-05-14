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

static uint32_t mapExe(uint32_t offset) {
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

  if ((offset >= 0x00401000) && (offset < (0x00401000 + 0x000aa750))) {
    // .text
    offset -= 0x00401000;
    offset += 0x00000400;
  } else if ((offset >= 0x004ac000) && (offset < (0x004ac000 + 0x000054a2))) {
    // .rdata
    offset -= 0x004ac000;
    offset += 0x000aac00;
  } else if ((offset >= 0x004b2000) && (offset < (0x004b2000 + 0x00023600))) {
    offset -= 0x004b2000;
    offset += 0x000b0200;
  } else {
    // .rsrc or unsupported
    printf("Unknown offset: 0x%08X\n", offset);
    //assert(false);
    offset -= 0x00ece000;
    offset += 0x000d3800;
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

int main(int argc, char* argv[]) {

  FILE* f = fopen(argv[1], "rb+");
  assert(f != NULL);

  // Read timestamp of binary to see which base version this is
  uint32_t timestamp;
  fseek(f, 216, SEEK_SET); //FIXME: Parse headers properly
  fread(&timestamp, 4, 1, f);

  // Now set the correct pointers for this binary
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
  uint32_t image_base = 0x400000; // FIXME: Read

  uint32_t memory_size;
  uint32_t memory_offset;
  fseek(f, 576 + 8, SEEK_SET);
  fread(&memory_size, 4, 1, f);
  fread(&memory_offset, 4, 1, f);
  memory_offset += image_base;

  uint32_t file_size;
  uint32_t file_offset;
  fread(&file_size, 4, 1, f);
  fread(&file_offset, 4, 1, f);

  // Go to the end of each section
  file_offset += file_size;
  memory_offset += memory_size;

  // Patchup the section size
  file_size += patch_size;
  memory_size += patch_size;

  // Write the new size
  fseek(f, 576 + 8, SEEK_SET);
  fwrite(&memory_size, 4, 1, f);
  fseek(f, 576 + 16, SEEK_SET);
  fwrite(&file_size, 4, 1, f);

  // Extend the file to the size
  fseek(f, file_offset, SEEK_SET);
  while(ftell(f) < (file_offset + file_size)) {
    uint8_t dummy = 0x00;
    fwrite(&dummy, 1, 1, f);
  }

  // Fix section characteristics
  uint32_t characteristics;
  fseek(f, 576 + 36, SEEK_SET);
  fread(&characteristics, 4, 1, f);
  characteristics |= 0x20000000; // Executable
  characteristics |= 0x80000000; // Writeable
  fseek(f, -4, SEEK_CUR);
  fwrite(&characteristics, 4, 1, f);

  printf("File offset: 0x%08X (Memory: 0x%08X, size = 0x%X/0x%X)\n", file_offset, memory_offset, file_size, memory_size);


  memory_offset = patchTextureTable(f, memory_offset, 0x4BF91C, 0x42D745, 0x42D753, 512, 1024, "font0");  
  memory_offset = patchTextureTable(f, memory_offset, 0x4BF7E4, 0x42D786, 0x42D794, 512, 1024, "font1");
  memory_offset = patchTextureTable(f, memory_offset, 0x4BF84C, 0x42D7C7, 0x42D7D5, 512, 1024, "font2");
  memory_offset = patchTextureTable(f, memory_offset, 0x4BF8B4, 0x42D808, 0x42D816, 512, 1024, "font3");
  //FIXME: font4

#if 0
  dumpTextureTable(f, 0x4BF91C, 3, 0, 64, 128, "font0");
  dumpTextureTable(f, 0x4BF7E4, 3, 0, 64, 128, "font1");
  dumpTextureTable(f, 0x4BF84C, 3, 0, 64, 128, "font2");
  dumpTextureTable(f, 0x4BF8B4, 3, 0, 64, 128, "font3");
  dumpTextureTable(f, 0x4BF984, 3, 0, 64, 128, "font4");
#endif

  fclose(f);

  return 0;
}

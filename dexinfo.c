/*
 * dexinfo - a very rudimentary dex file parser
 *
 * Copyright (C) 2014 Keith Makan (@k3170Makan)
 * Copyright (C) 2012-2013 Pau Oliva Fora (@pof)
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>

#define VERSION "0.1"

#define MAX_BUFSIZE 1024

#ifdef PYDEXINFO

size_t dexinfo_read(uint8_t * buf, size_t len);
void   dexinfo_seek(off_t offset, int whence);

#define psseek(f, off, whence)		dexinfo_seek(off, whence)
#define psread(buf, len, nmemb, f)	dexinfo_read((uint8_t *)buf, (len * nmemb))

#define psprintf( ... )					\
{							\
	char tmp[MAX_BUFSIZE];				\
	snprintf(tmp, MAX_BUFSIZE, __VA_ARGS__);	\
	printbuf_write(tmp);				\
}

static char * printbuf = NULL;
static size_t printbuf_len = 0;

static void printbuf_write(char * data)
{
	size_t len = strlen(data);

	if ((printbuf = realloc(printbuf, printbuf_len + len)) == NULL)
	{
		printf("Error allocating output buffer");

		return;
	}

	memset(printbuf + printbuf_len, 0, len);
	memcpy(printbuf + printbuf_len, data, len);
	printbuf_len += len;
}

#else

#define psseek(f, off, whence)		fseek(f, off, whence);

#define psread(buf, len, nmemb, f)	fread(buf, len, nmemb, f)

#define psprintf( ... )					\
		printf( __VA_ARGS__ );
#endif

typedef uint8_t             u1;
typedef uint16_t            u2;
typedef uint32_t            u4;
typedef uint64_t            u8;
typedef int8_t              s1;
typedef int16_t             s2;
typedef int32_t             s4;
typedef int64_t             s8;


typedef struct {
	char dex[3];
	char newline[1];
	char ver[3];
	char zero[1];
} dex_magic;

typedef struct {
	dex_magic magic;
	u4 checksum[1];
	unsigned char signature[20];
	u4 file_size[1];
	u4 header_size[1];
	u4 endian_tag[1];
	u4 link_size[1];
	u4 link_off[1];
	u4 map_off[1];
	u4 string_ids_size[1];
	u4 string_ids_off[1];
	u4 type_ids_size[1];
	u4 type_ids_off[1];
	u4 proto_ids_size[1];
	u4 proto_ids_off[1];
	u4 field_ids_size[1];
	u4 field_ids_off[1];
	u4 method_ids_size[1];
	u4 method_ids_off[1];
	u4 class_defs_size[1];
	u4 class_defs_off[1];
	u4 data_size[1];
	u4 data_off[1];
} dex_header;

typedef struct {
	u4 class_idx[1];
	u4 access_flags[1];
	u4 superclass_idx[1];
	u4 interfaces_off[1];
	u4 source_file_idx[1];
	u4 annotations_off[1];
	u4 class_data_off[1];
	u4 static_values_off[1];
} class_def_struct;

typedef struct {
	u2 class_idx[1];
	u2 proto_idx[1];
	u4 name_idx[1];
} method_id_struct;

typedef struct {
	u4 string_data_off[1];
} string_id_struct;

typedef struct {
	u4 descriptor_idx[1];
} type_id_struct;

typedef struct {
	u4 descriptor_idx[1];
} proto_id_struct;

/*names for the access flags*/
const char * ACCESS_FLAG_NAMES[20] = {
    "public",
    "private",
    "protected",
    "static",
    "final",
    "synchronized",
    "super",
    "volatile",
    "bridge",
    "transient",
    "varargs",
    "native",
    "interface",
    "abstract",
    "strict",
    "synthetic",
    "annotation",
    "enum",
    "constructor",
    "declared_synchronized"};
/*values for the access flags, this and the preceeding list are used as a lookup dictionary*/
const u4 ACCESS_FLAG_VALUES[20] = {
    0x00000001,
    0x00000002,
    0x00000004,
    0x00000008,
    0x00000010,
    0x00000020,
    0x00000020,
    0x00000040,
    0x00000040,
    0x00000080,
    0x00000080,
    0x00000100,
    0x00000200,
    0x00000400,
    0x00000800,
    0x00001000,
    0x00002000,
    0x00004000,
    0x00010000,
    0x00020000};

const u4 NO_INDEX = 0xffffffff;

int readUnsignedLeb128(u1** pStream)
{
/* taken from dalvik's libdex/Leb128.h */
    u1* ptr = *pStream;
    int result = *(ptr++);

    if (result > 0x7f) {
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur > 0x7f) {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur > 0x7f) {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur > 0x7f) {
                    /*
                     * Note: We don't check to see if cur is out of
                     * range here, meaning we tolerate garbage in the
                     * high four-order bits.
                     */
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }

    *pStream = ptr;
    return result;
}

int uleb128_value(u1* pStream)
{
    u1* ptr = pStream;
    int result = *(ptr++);

    if (result > 0x7f) {
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur > 0x7f) {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur > 0x7f) {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur > 0x7f) {
                    /*
                     * Note: We don't check to see if cur is out of
                     * range here, meaning we tolerate garbage in the
                     * high four-order bits.
                     */
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    return result;
}


size_t len_uleb128(unsigned long n)
{
    static unsigned char b[32];
    size_t i;

    i = 0;
    do
    {
        b[i] = n & 0x7F;
        if (n >>= 7)
            b[i] |= 0x80;
    }
    while (b[i++] & 0x80);
    return i;
}
/*wrote this to avoid dumping the leb128 parsing and grabbing code all over the place ;)*/
void 
printUnsignedLebValue(char *format,
							u1 *stringData,
							size_t offset,
							FILE* DexFile){

	u1 *uLebBuff;
	int uLebValue, uLebValueLength;

	psseek(DexFile,offset,SEEK_SET); /*move position to the string in data section*/
	uLebBuff = malloc(10*sizeof(u1))	;
	psread(uLebBuff,1,sizeof(uLebBuff),DexFile);

	uLebValue = uleb128_value(uLebBuff);	
	uLebValueLength = len_uleb128(uLebValue);
	stringData = malloc(uLebValue * sizeof(u1)+1);
	
	psseek(DexFile, offset+uLebValueLength ,SEEK_SET);
	psread(stringData,1,uLebValue,DexFile);

	stringData[uLebValue]='\0';	

	psprintf(format,stringData);
	free(uLebBuff);
}
/*this allows us to print ACC_FLAGS symbolically*/
void parseAccessFlags(u4 flags){
	int i = 0;
	if (flags){
		for (;i<20;i++){
			if (flags & ACCESS_FLAG_VALUES[i]){
					psprintf(" %s ",ACCESS_FLAG_NAMES[i]);	
			}
		}	
	}
	psprintf("\n");
}
/*not entirely sure how I should use these methods, as is they are only usefull for printing values, and don't return them :/
though as a tradeoff I've made the methods manipulate the string data in place, so the conversion to returning them would be easy */
/*Generic methods for printing types*/
void
printStringValue(string_id_struct *strIdList,
				u4 offset_pointer,
				FILE* DexFile,
				u1* stringData,
				char* format){

	size_t strIdOff;
	if (offset_pointer){
		//printf("strIdOff = %p\n", strIdList[offset_pointer].string_data_off);
		strIdOff = *strIdList[offset_pointer].string_data_off; /*get the offset to the string in the data section*/
		/*would be cool if we have a RAW mode, with only hex unparsed data, and a SYMBOLIC mode where all the data is parsed and interpreted */
		printUnsignedLebValue(format,stringData,strIdOff,DexFile);
	}
	else{
		psprintf("none\n");
	}
	free(stringData);
	stringData=NULL;

}

void
printTypeDesc(string_id_struct *strIdList,
				type_id_struct* typeIdList,
				u4 offset_pointer,
				FILE* DexFile,
				u1* stringData,
				char* format){

	size_t strIdOff;
	if (offset_pointer){
		strIdOff = *strIdList[*typeIdList[offset_pointer].descriptor_idx].string_data_off; /*get the offset to the string in the data section*/
		/*would be cool if we have a RAW mode, with only hex unparsed data, and a SYMBOLIC mode where all the data is parsed and interpreted */
		printUnsignedLebValue(format,stringData,strIdOff,DexFile);
	}
	else{
		psprintf("none\n");
	}
	free(stringData);
	stringData=NULL;

}
void 
printClassFileName(string_id_struct *strIdList, 
				class_def_struct classDefItem,
				FILE *DexFile,
				u1* stringData){

	size_t strIdOff;
	if (classDefItem.source_file_idx){
		strIdOff = *strIdList[*classDefItem.source_file_idx].string_data_off; /*get the offset to the string in the data section*/
		printUnsignedLebValue("(%s)\n",stringData,strIdOff,DexFile);
	}
	else{
		psprintf("none\n");
	}
	free(stringData);
	stringData=NULL;
	
}
void
printTypeDescForClass(string_id_struct *strIdList, 
				type_id_struct* typeIdList,
				class_def_struct classDefItem,
				FILE *DexFile,
				u1* stringData){
	size_t strIdOff;
	if (classDefItem.class_idx){
	strIdOff = *strIdList[*typeIdList[*classDefItem.class_idx].descriptor_idx].string_data_off; /*get the offset to the string in the data section*/
	printUnsignedLebValue("%s\n",stringData,strIdOff,DexFile);
	free(stringData);
	stringData=NULL;
	}
	else{
		psprintf("none\n");
	}
}
void parseClass(){

}
void parseHeader(){

}
void help_show_message()
{
	fprintf(stderr, "Usage: dexinfo <file.dex> [options]\n");
	fprintf(stderr, " options:\n");
	fprintf(stderr, "    -V             print verbose information\n");
}

char * dexinfo(char * dexfile, int DEBUG)
{
	// char *dexfile;
	FILE *input = NULL;
	size_t offset, offset2;
	ssize_t len;
	int i,c;
	// int DEBUG=0;

	int static_fields_size;
	int instance_fields_size;
	int direct_methods_size;
	int virtual_methods_size; 

	int field_idx_diff;
	int field_access_flags;

	int method_idx_diff;
	int method_access_flags;
	int method_code_off;

	int key;

	dex_header header;
	class_def_struct class_def_item;
	u1* buffer;
	u1* buf;
	u1* buffer_pos;
	u1* str=NULL;

	method_id_struct method_id_item;
	method_id_struct* method_id_list;

	string_id_struct string_id_item;
	string_id_struct* string_id_list;

	type_id_struct type_id_item;
	type_id_struct* type_id_list;

	int size_uleb, size_uleb_value;

#ifdef PYDEXINFO
	if (!printbuf)
	{
	}
	else
	{
		free(printbuf);
		printbuf = NULL;
		printbuf_len = 0;
	}
#endif

	psprintf ("\n=== dexinfo %s - (c) 2012-2013 Pau Oliva Fora\n\n", VERSION);

#ifndef PYDEXINFO
	input = fopen(dexfile, "rb");
	if (input == NULL) {
		fprintf(stderr, "ERROR: Can't open dex file!\n");
		perror(dexfile);
#ifndef PYDEXINFO
			exit(1);
#else
			return NULL;
#endif
	}
#endif

	/* print dex header information */
        psprintf ("[] Dex file: %s\n\n",dexfile);

	memset(&header, 0, sizeof(header));
	memset(&class_def_item, 0, sizeof(class_def_item));

	psread(&header, 1, sizeof(header), input);

	psprintf ("[] DEX magic: ");
	for (i=0;i<3;i++) psprintf("%02X ", header.magic.dex[i]);
	psprintf("%02X ", *header.magic.newline);
	for (i=0;i<3;i++) psprintf("%02X ", header.magic.ver[i]);
	psprintf("%02X ", *header.magic.zero);
	psprintf ("\n");

	if ( (strncmp(header.magic.dex,"dex",3) != 0) || 
	     (strncmp(header.magic.newline,"\n",1) != 0) || 
	     (strncmp(header.magic.zero,"\0",1) != 0 ) ) {
		fprintf (stderr, "ERROR: not a dex file\n");
#ifndef PYDEXINFO
			fclose(input);

			exit(1);
#else
			return NULL;
#endif
	}

	psprintf ("[] DEX version: %s\n", header.magic.ver);
	if (strncmp(header.magic.ver,"035",3) != 0) {
		fprintf (stderr,"Warning: Dex file version != 035\n");
	}

	psprintf ("[] Adler32 checksum: 0x%x\n", *header.checksum);

	psprintf ("[] SHA1 signature: ");
	for (i=0;i<20;i++) psprintf("%02x", header.signature[i]);
	psprintf("\n");

	if (DEBUG) {
		psprintf ("[] File size: %d bytes\n", *header.file_size);
		psprintf ("[] DEX Header size: %d bytes (0x%x)\n", *header.header_size, *header.header_size);
	}

	if (*header.header_size != 0x70) {
		fprintf (stderr,"Warning: Header size != 0x70\n");
	}

	if (DEBUG) psprintf("[] Endian Tag: 0x%x\n", *header.endian_tag);
	if (*header.endian_tag != 0x12345678) {
		fprintf (stderr,"Warning: Endian tag != 0x12345678\n");
	}

	if (DEBUG) {
		psprintf("[] Link size: %d\n", *header.link_size);
		psprintf("[] Link offset: 0x%x\n", *header.link_off);
		psprintf("[] Map list offset: 0x%x\n", *header.map_off);
		psprintf("[] Number of strings in string ID list: %d\n", *header.string_ids_size);
		psprintf("[] String ID list offset: 0x%x\n", *header.string_ids_off);
		psprintf("[] Number of types in the type ID list: %d\n", *header.type_ids_size);
		psprintf("[] Type ID list offset: 0x%x\n", *header.type_ids_off);
		psprintf("[] Number of items in the method prototype ID list: %d\n", *header.proto_ids_size);
		psprintf("[] Method prototype ID list offset: 0x%x\n", *header.proto_ids_off);
		psprintf("[] Number of item in the field ID list: %d\n", *header.field_ids_size);
		psprintf("[] Field ID list offset: 0x%x\n", *header.field_ids_off);
		psprintf("[] Number of items in the method ID list: %d\n", *header.method_ids_size);
		psprintf("[] Method ID list offset: 0x%x\n", *header.method_ids_off);
		psprintf("[] Number of items in the class definitions list: %d\n", *header.class_defs_size);
		psprintf("[] Class definitions list offset: 0x%x\n", *header.class_defs_off);
		psprintf("[] Data section size: %d bytes\n", *header.data_size);
		psprintf("[] Data section offset: 0x%x\n", *header.data_off);
	}

	psprintf("\n[] Number of classes in the archive: %d\n", *header.class_defs_size);

	/* parse the strings */
	string_id_list = malloc(*header.string_ids_size*sizeof(string_id_item));
	psseek(input, *header.string_ids_off, SEEK_SET);
	psread(string_id_list, 1, *header.string_ids_size*sizeof(string_id_item), input);

	/* parse the types */
	type_id_list = malloc(*header.type_ids_size*sizeof(type_id_item));
	psseek(input, *header.type_ids_off, SEEK_SET);
	psread(type_id_list, 1, *header.type_ids_size*sizeof(type_id_item), input);

	/* parse methods */
	method_id_list = malloc(*header.method_ids_size*sizeof(method_id_item));
	psseek(input, *header.method_ids_off, SEEK_SET);
	psread(method_id_list, 1, *header.method_ids_size*sizeof(method_id_item), input);

#if 0
	/* strings */
	for (i=0;i < (*header.string_ids_size)*(sizeof(string_id_item)) ;i+=4) {
		 psprintf("string_id_list[%d] (%x) = \n", i/4, *string_id_list[i/4].string_data_off);
	}
#endif
#ifdef PYDEXINFO

	/* methods */
	for (i=0;i<sizeof(method_id_item)*(*header.method_ids_size);i+=8) {
		// psprintf ("method_id_list[%d]class=%x\n", i/8, *method_id_list[i/8].class_idx);
		// psprintf ("method_id_list[%d]proto=%x\n", i/8, *method_id_list[i/8].proto_idx);
		// psprintf ("method_id_list[%d]name=%x\n", i/8, *method_id_list[i/8].name_idx);
		printStringValue(string_id_list, *method_id_list[i/8].name_idx, input, str, "MethodVal %s\n");
	}

#endif

	/*Parse class definitions*/
	for (c=1; c <= (int)*header.class_defs_size; c++) { /*run through all the class */
		// change the position to the class_def_struct of each class
		offset = *header.class_defs_off + ((c-1)*sizeof(class_def_item)); /*get the offset for this class definition*/
		psseek(input, offset, SEEK_SET);
		psprintf("[] Class %d ", c);
		psread(&class_def_item, 1, sizeof(class_def_item), input); /*read the class definition from the input*/
		/* print class filename */
		if (*class_def_item.source_file_idx != 0xffffffff) {
			printClassFileName(string_id_list,class_def_item,input,str);
		} else {
			psprintf ("(No index): ");
		}

		if (DEBUG) {
			psprintf("\n");
			/* print type id */
			psprintf("\tclass_idx='0x%x':", *class_def_item.class_idx);
			printTypeDescForClass(string_id_list,type_id_list,class_def_item,input,str);
			psprintf("\taccess_flags='0x%x':", *class_def_item.access_flags); /*need to interpret this*/
			parseAccessFlags(*class_def_item.access_flags);
			psprintf("\tsuperclass_idx='0x%x':", *class_def_item.superclass_idx);
			printTypeDesc(string_id_list,type_id_list,*class_def_item.superclass_idx,input,str,"%s\n");
			psprintf("\tinterfaces_off='0x%x'\n", *class_def_item.interfaces_off); /*need to look this up in the DexTypeList*/
			psprintf("\tsource_file_idx='0x%x'\n", *class_def_item.source_file_idx);
            if (*class_def_item.source_file_idx != NO_INDEX) 
			printStringValue(string_id_list,*class_def_item.source_file_idx,input,str,"%s\n"); //causes a seg fault on some dex files
            // The seg fault was because there was no index value on the
            // class_def_item.scource_fie_idx
		/*should implement decoding the annotations directory items, we can use this to idenfiy Javascript interface accessible methods*/
			psprintf("\tannotations_off=0x%x\n", *class_def_item.annotations_off);
			psprintf("\tclass_data_off=0x%x (%d)\n", *class_def_item.class_data_off, *class_def_item.class_data_off);
			psprintf("\tstatic_values_off=0x%x (%d)\n", *class_def_item.static_values_off, *class_def_item.static_values_off);
		}

		// change position to class_data_off
		if (*class_def_item.class_data_off == 0) {
			if (DEBUG) {
				psprintf ("\t0 static fields\n");
				psprintf ("\t0 instance fields\n");
				psprintf ("\t0 direct methods\n");
			} else {
				psprintf ("0 direct methods, 0 virtual methods\n");
			}
			continue;
		} else {

			offset = *class_def_item.class_data_off;
			psseek(input, offset, SEEK_SET);
		}

		len = *header.map_off - offset;
		if (len < 1) {
			len = *header.file_size - offset;
			if (len < 1) {
				fprintf(stderr, "ERROR: invalid file length in dex header?\n");
				fclose(input);
#ifndef PYDEXINFO
					exit(1);
#else
					return NULL;
#endif
			}
		}

		buffer = malloc(len * sizeof(u1));
		if (buffer == NULL) {
			fprintf(stderr, "ERROR: could not allocate memory!\n");
			fclose(input);
#ifndef PYDEXINFO
				exit(1);
#else
				return NULL;
#endif
		}
		buffer_pos=buffer;
		memset(buffer, 0, len);
		psread(buffer, 1, len, input);

		// from now on we continue on memory, as we have to parse uleb128
		static_fields_size = readUnsignedLeb128(&buffer);
		instance_fields_size = readUnsignedLeb128(&buffer);
		direct_methods_size = readUnsignedLeb128(&buffer);
		virtual_methods_size = readUnsignedLeb128(&buffer);

		if (DEBUG) psprintf ("\t%d static fields\n", static_fields_size);

		for (i=0;i<static_fields_size;i++) {
			field_idx_diff = readUnsignedLeb128(&buffer);
			field_access_flags = readUnsignedLeb128(&buffer);
			if (DEBUG) {
				psprintf ("\t\t[%d]|--field_idx_diff='0x%x'\n",i, field_idx_diff);
				//printTypeDesc(string_id_list,type_id_list,field_idx_diff,input,str," %s\n");
				psprintf ("\t\t    |--field_access_flags='0x%x'",field_access_flags);
				parseAccessFlags(field_access_flags);
			}
		}

		if (DEBUG) psprintf ("\t%d instance fields\n", instance_fields_size);

		for (i=0;i<instance_fields_size;i++) {
			field_idx_diff = readUnsignedLeb128(&buffer);
			field_access_flags = readUnsignedLeb128(&buffer);
			if (DEBUG) {
				psprintf ("\t\t[%d]|--field_idx_diff='0x%x'\n", i,field_idx_diff);
				//printTypeDesc(string_id_list,type_id_list,field_idx_diff,input,str,"%s\n");
				psprintf ("\t\t    |--field_access_flags='0x%x' :",field_access_flags);
				parseAccessFlags(field_access_flags);
			}
		}

		if (!DEBUG) psprintf ("%d direct methods, %d virtual methods\n", direct_methods_size, virtual_methods_size);


		if (DEBUG) psprintf ("\t%d direct methods\n", direct_methods_size);

		key=0;
		for (i=0;i<direct_methods_size;i++) {
			method_idx_diff = readUnsignedLeb128(&buffer);
			method_access_flags = readUnsignedLeb128(&buffer);
			method_code_off = readUnsignedLeb128(&buffer);

			/* methods */
			if (key == 0) key=method_idx_diff;
			else key += method_idx_diff;

			u2 class_idx=*method_id_list[key].class_idx;
			u2 proto_idx=*method_id_list[key].proto_idx;
			u4 name_idx=*method_id_list[key].name_idx;

			/* print method name ... should really do this stuff through a common function, its going to be annoying to debug this...:/ */
			offset2=*string_id_list[name_idx].string_data_off;
			psseek(input, offset2, SEEK_SET);

			buf = malloc(10 * sizeof(u1));
			psread(buf, 1, sizeof(buf), input);
			size_uleb_value = uleb128_value(buf);
			size_uleb=len_uleb128(size_uleb_value);
			str = malloc(size_uleb_value * sizeof(u1)+1);
			// offset2: on esta el tamany (size_uleb_value) en uleb32 de la string, seguit de la string
			psseek(input, offset2+size_uleb, SEEK_SET);
			psread(str, 1, size_uleb_value, input);
			str[size_uleb_value]='\0';

			psprintf ("\tdirect method %d = %s\n",i+1, str);
			free(str);
			str=NULL;
			if (DEBUG) {
				psprintf("\t\tmethod_code_off=0x%x\n", method_code_off);
				psprintf("\t\tmethod_access_flags='0x%x'\n", method_access_flags);
				//parseAccessFlags(method_access_flags);	
				psprintf("\t\tclass_idx='0x%x'\n", class_idx);
				//printTypeDesc(string_id_list,type_id_list,class_idx,input,str," %s\n");
				psprintf("\t\tproto_idx=0x%x\n", proto_idx);
			}
		}

		if (DEBUG) psprintf ("\t%d virtual methods\n", virtual_methods_size);

		key=0;
		for (i=0;i<virtual_methods_size;i++) {
			method_idx_diff = readUnsignedLeb128(&buffer);
			method_access_flags = readUnsignedLeb128(&buffer);
			method_code_off = readUnsignedLeb128(&buffer);

			/* methods */
			if (key == 0) key=method_idx_diff;
			else key += method_idx_diff;

			u2 class_idx=*method_id_list[key].class_idx;
			u2 proto_idx=*method_id_list[key].proto_idx;
			u4 name_idx=*method_id_list[key].name_idx;
			
			/* print method name */
			offset2=*string_id_list[name_idx].string_data_off;
			//printStringValue(string_id_list,name_idx,input,str,"%s\n");
			psseek(input, offset2, SEEK_SET);

			buf = malloc(10 * sizeof(u1));
			psread(buf, 1, sizeof(buf), input);
			size_uleb_value = uleb128_value(buf);
			size_uleb=len_uleb128(size_uleb_value);
			str = malloc(size_uleb_value * sizeof(u1)+1);
			// offset2: on esta el tamany (size_uleb_value) en uleb32 de la string, seguit de la string 
			psseek(input, offset2+size_uleb, SEEK_SET);
			psread(str, 1, size_uleb_value, input);
			str[size_uleb_value]='\0';

			psprintf ("\tvirtual method %d = %s\n",i+1, str);
			free(str);
			str=NULL;
			if (DEBUG) {
				psprintf("\t\tmethod_code_off=0x%x\n", method_code_off);
				psprintf("\t\tmethod_access_flags='0x%x'\n", method_access_flags);
				//parseAccessFlags(method_access_flags);	
				psprintf("\t\tclass_idx=0x%x\n", class_idx);
				psprintf("\t\tproto_idx=0x%x\n", proto_idx);
			}

		}

		free(buffer_pos);
	}

	free(type_id_list);
	free(method_id_list);
	free(string_id_list);

#ifdef PYDEXINFO
	return printbuf;
#else
	fclose(input);
	return NULL;
#endif
}

int main(int argc, char *argv[])
{
	char *dexfile;
	int DEBUG=0;
	int c;

	if (argc < 2) {
		help_show_message();
		return 1;
	}

	dexfile=argv[1];

        while ((c = getopt(argc, argv, "V")) != -1) {
                switch(c) {
     		case 'V':
			DEBUG=1;
			break;
                default:
                        help_show_message();
                        return 1;
                }
        }

        dexinfo(dexfile, DEBUG);

        return 0;
}

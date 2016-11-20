#include "../include/utfconverter.h"
#include <sys/stat.h>
#include <stdlib.h>

char* filename ;
char* output_filename;
FILE *output_file;
endianness source;
endianness conversion;
int verbosity;
int is8to16;
int bom_present;

int ascii_count;
int surrogate_count;
int glyph_count;

int main(int argc,char** argv)
{
	/* file descriptor*/
	int fd;

	/* Temporary variables, to be used later*/
	unsigned char buf[4]; 
	int rv;
	Glyph* glyph;
	void* memset_return;
	
	/* Byte order mark(bom) of the glyph */
	Glyph* bom;

	struct stat buffer;
	unsigned int codepoint;

	/*Records to print during verbosity level 1*/
	char *symlinkpath;
	char actualpath [4096+1];
	char *absolute_path;

	/* Buffer for host name*/
	char hostname[128]; 

	/* for measuring the Statistics(Stats) */
	ascii_count=0;
	surrogate_count=0;
	glyph_count=0;

	verbosity = 0;
	/*set is 8 to 16 conversion to false*/
	is8to16 = 0;
	/*initialize code point*/
	codepoint =0;
	/* this shows that bom is not present in the file so you can write bom*/
	bom_present =0;

	/* After calling parse_args(), filename and conversion should be set. */
	parse_args(argc, argv); 
	glyph = (Glyph*)calloc(1, sizeof(Glyph)); 
	
	/* Open the file*/
	fd = open(filename, O_RDONLY);

	/*If open failfs*/
	if(fd == -1){
		printf("%s\n","Invalid input file");
		print_help();
		free(filename);
		exit(EXIT_FAILURE);
	} 

	/*Find the Byte order mark(bom) if present*/
	if(output_filename != NULL){
		output_file = fopen(output_filename, "r");
		
		unsigned char a;
		unsigned char b;

		if(output_file){
			if(fread(&a,1,1,output_file) != 0 && (fread(&b,1,1,output_file) != 0 )){

				/*check for BOM for UTF-16LE encoding*/
				if(a == 0xff && b==0xfe ){
					if(conversion == LITTLE ){
						bom_present = 1;
					}else{
						exit(EXIT_FAILURE);
					}
				}
				/*check for BOM for UTF-16BE encoding*/
				else if(a == 0xfe && b==0xff){
					if(conversion == BIG ){
						bom_present = 1;
					}else{
						exit(EXIT_FAILURE);
					}
				}else{
					exit(EXIT_FAILURE);
				}

			}

			fclose(output_file);
		}		

	} else {

		output_file = NULL;
	}

	rv = 0; 
		
	if(bom_present==0 && output_filename != NULL){
		output_file = fopen(output_filename,"w");
	}else if(bom_present==1 && output_filename != NULL){
		output_file = fopen(output_filename,"a");
	}	

	/* Find out what is the encoding of input file */
	if((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1){ 

		if(buf[0] == 0xff && buf[1] == 0xfe){

			/*file is little endian*/
			source = LITTLE;

		} else if(buf[0] == 0xfe && buf[1] == 0xff){

			/*file is big endian*/
			source = BIG; 

		} else {

			/*if file has no BOM*/
			if((rv = read(fd,&buf[2],1))== 1){

				if(buf[0] == 0xef && buf[1] == 0xbb && buf[2]==0xbf){
					/* File is of UTF-8 format*/
					is8to16 = 1;
				}

			}else{

				free(&glyph->bytes); 
				fprintf(stderr, "File has no BOM.\n");
				quit_converter(NO_FD); 
				exit(EXIT_FAILURE);
			}
		}
		
		memset_return =  memset(glyph, 0, sizeof(Glyph));

		/* Memory write failed, recover from it: */
		if(memset_return == NULL){
			memset(glyph, 0, sizeof(Glyph));
		}
	}
	

	/*TO change the BOM of the file if there is no BOM present else skip*/

	if(bom_present == 0){	
		bom = malloc(sizeof(Glyph));
		bom -> surrogate = false;

		if(conversion == LITTLE){
			/* BOM for big indian*/

			bom->bytes[0] = 0xff;
			bom->bytes[1] = 0xfe;

			bom->end =LITTLE;
		}
		else if(is8to16 ==1 && conversion == BIG){

			bom->bytes[0] = 0xff;
			bom->bytes[1] = 0xfe;

			bom->end =LITTLE;
		}
		else if(conversion == BIG){
			/* BOM for little indian*/
			
			bom->bytes[0]= 0xfe;
			bom->bytes[1]= 0xff;

			bom->end = BIG;
		} 

		/*To write the bom glyph */
		glyph_count+=1;

		write_glyph(bom);

	}

	if(is8to16 == 1){

		/* Now deal with the rest of the bytes.*/
		while((rv = read(fd, &buf[0], 1)) == 1 ){

			if((buf[0] >> 7) == 0x0){
				codepoint = buf[0];
			}else{

				if((buf[0] >> 3) == 0x1e){

					rv = read(fd, &buf[1], 1);
					rv = read(fd, &buf[2], 1);
					rv = read(fd, &buf[3], 1);

					/*first mask all the useless bytes*/
					buf[0] &= 0x7;
					buf[1] &= 0x3f;
					buf[2] &= 0x3f;
					buf[3] &= 0x3f;

					/*now get the code point*/
					codepoint = buf[0];
					codepoint = (codepoint << 6);
					codepoint += buf[1];
					codepoint = (codepoint << 6);
					codepoint += buf[2];
					codepoint = (codepoint << 6);
					codepoint += buf[3];

				}

				else if((buf[0] >> 4) == 0xe){

					rv = read(fd, &buf[1], 1);
					rv = read(fd, &buf[2], 1);

					/*first mask all the useless bytes 0*/
					buf[0] &= 0xf;
					buf[1] &= 0x3f;
					buf[2] &= 0x3f;

					/*now get the code point*/
					codepoint = buf[0];
					codepoint = (codepoint << 6);
					codepoint += buf[1];
					codepoint = (codepoint << 6);
					codepoint += buf[2];


				}
				
				else if((buf[0] >> 5) == 0x6){

					rv = read(fd, &buf[1], 1);

					/*mask all useless bytes*/
					buf[0] &= 0x1f;
					buf[1] &= 0x3f;
					
					/*code get the code point*/
					codepoint = buf[0];
					codepoint = (codepoint << 6);
					codepoint += buf[1];


				}

			}

			if(codepoint<128){
				ascii_count+=1;
			}

			/*check whether the code point is surrogate or nah*/
			if(codepoint >= 0x10000){

				glyph->surrogate=true;
			}else{
				glyph->surrogate=false;				
			}



			write_glyph(fill_utf8_glyph(glyph, codepoint));

			memset_return= memset(glyph, 0, sizeof(Glyph));

	 	       /*Memory write failed, recover from it: */
	 	       if(memset_return == NULL){
		        	memset(glyph, 0, sizeof(Glyph));
	 	       }
		}

	}else{

		/* Now deal with the rest of the bytes.*/
		while((rv = read(fd, &buf[0], 1)) == 1 &&  (rv = read(fd, &buf[1], 1)) == 1){
		

		write_glyph(fill_glyph(glyph, buf, source));

		memset_return= memset(glyph, 0, sizeof(Glyph));

	        /* Memory write failed, recover from it: */
	        if(memset_return == NULL){
		        
		        memset(glyph, 0, sizeof(Glyph));
	        }
		}
	}

	if(verbosity == 1){
		/*fill the struct*/
		stat(filename,&buffer);

		fprintf(stderr,"Input file size: %zu bytes\n",(size_t)buffer.st_size );

		/*for printing the absolutte path of the file*/
		symlinkpath = strdup(filename);
		absolute_path = realpath(symlinkpath, actualpath);

		fprintf(stderr,"Input file path: %s\n", absolute_path);

		if(source == BIG){
			fprintf(stderr,"Input file encoding: UTF-16BE\n");
		}else if(source == LITTLE){
			fprintf(stderr,"Input file encoding: UTF-16LE\n");
		}else{
			fprintf(stderr,"Input file encoding: UTF-8\n");
		}

		if(conversion == BIG){
			fprintf(stderr,"Output encoding: UTF-16BE\n");
		}else if(conversion == LITTLE){
			fprintf(stderr,"Output encoding: UTF-16LE\n");
		}else{
			fprintf(stderr,"Output encoding: UTF-8\n");
		}

		gethostname(hostname, sizeof(hostname));
		fprintf(stderr,"Hostmachine: %s\n",hostname );

	}else if(verbosity>1){

		/*fill the struct*/
		stat(filename,&buffer);

		fprintf(stderr,"Input file size: %zu bytes\n",(size_t)buffer.st_size );

		/*for printing the absolutte path of the file*/
		symlinkpath = strdup(filename);
		absolute_path = realpath(symlinkpath, actualpath);

		fprintf(stderr,"Input file path: %s\n", absolute_path);

		if(source == BIG){
			fprintf(stderr,"Input file encoding: UTF-16BE\n");
		}else if(source == LITTLE){
			fprintf(stderr,"Input file encoding: UTF-16LE\n");
		}else{
			fprintf(stderr,"Input file encoding: UTF-8\n");
		}

		if(conversion == BIG){
			fprintf(stderr,"Output encoding: UTF-16BE\n");
		}else if(conversion == LITTLE){
			fprintf(stderr,"Output encoding: UTF-16LE\n");
		}else{
			fprintf(stderr,"Output encoding: UTF-8\n");
		}

		gethostname(hostname, sizeof(hostname));
		fprintf(stderr,"Hostmachine: %s\n",hostname );
		fprintf(stderr,"ASCII: %f\n", ((float)ascii_count/glyph_count*100.0));
		fprintf(stderr,"Surrogates: %f%%\n", ((float)surrogate_count/glyph_count*100.0) );
		fprintf(stderr,"Glyphs: %d\n",glyph_count);
	}

	/*freeing memory to remove memory leaks*/
	free(glyph);
	free(bom);
	free(filename);

	quit_converter(NO_FD);
	return 0;
	
}


/* Swap the endianness of the glyph */
Glyph* swap_endianness( Glyph* glyph) {
	/* Use XOR to be more efficient with how we swap values. */
	glyph->bytes[0] ^= glyph->bytes[1];
	glyph->bytes[1] ^= glyph->bytes[0];
	if(glyph->surrogate){  /* If a surrogate pair, swap the next two bytes. */
		glyph->bytes[2] ^= glyph->bytes[3];
		glyph->bytes[3] ^= glyph->bytes[2];

	}
	glyph->end = conversion;
	return glyph;
}

/* Fill the glyph */
Glyph* fill_glyph(Glyph* glyph, unsigned char data[2] ,endianness end) {
	
	glyph_count+=1;

	if(source == conversion){
		glyph->bytes[0] = data[0];
		glyph->bytes[1] = data[1];

	}else{
		glyph->bytes[0] = data[1];
		glyph->bytes[1] = data[0];

	}

	glyph->end = end;

	return glyph;
}

/* Fill the utf8 glyph */
Glyph* fill_utf8_glyph(Glyph* glyph, unsigned int cp){

	glyph_count+=1;

	if(glyph->surrogate == false){

		glyph->bytes[1] = (unsigned char)(cp >> 8);
		glyph->bytes[1] &= 0xff;
		glyph->bytes[0] = (unsigned char)(cp & 0xff);

	}else{ 

		unsigned int v2= cp - 0x10000;

		unsigned int high_v = v2 >> 10;
		unsigned int w1 = 0xd800 + high_v; 

		unsigned int low_v =v2 & 0x3ff;
		unsigned int w2 = 0xdc00 + low_v;
		
		glyph->bytes[0]= w1 & 0xff;
		glyph->bytes[1]= w1 >> 8;

		glyph->bytes[2]= w2 & 0xff;
		glyph->bytes[3]= w2 >> 8;

	}

	glyph->end = LITTLE;
	return glyph;
}

/* Write to the glyph */
void write_glyph(Glyph* glyph){
	
	if(is8to16 == 1){

		if(conversion == BIG){

			unsigned char temp = glyph->bytes[0];
			glyph->bytes[0]=glyph->bytes[1];
			glyph->bytes[1]= temp;

			temp = glyph->bytes[2];
			glyph->bytes[2]  = glyph->bytes[3];
			glyph->bytes[3]=temp;

			if(glyph->surrogate){

				if(output_file){
					surrogate_count+=1;
					fwrite(&(glyph -> bytes),sizeof(unsigned char),SURROGATE_SIZE,output_file);
				}else{
					surrogate_count+=1;
					write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);
				}
			} else {

				if(output_file){
					fwrite(&(glyph -> bytes),sizeof(unsigned char),NON_SURROGATE_SIZE,output_file);
				}else{
					write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
				}
			}

		}else{

			if(glyph->surrogate){
				if(output_file){
					surrogate_count+=1;
					fwrite(&(glyph -> bytes),sizeof(unsigned char),SURROGATE_SIZE,output_file);
				}else{
					surrogate_count+=1;
					write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);
				}	
			} else {
				if(output_file){
					fwrite(&(glyph -> bytes),sizeof(unsigned char),NON_SURROGATE_SIZE,output_file);
				}else{
					write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
				}	
			}

		}

	}else{

		if(glyph->surrogate){
				if(output_file){
					surrogate_count+=1;
					fwrite(&(glyph -> bytes),sizeof(unsigned char),SURROGATE_SIZE,output_file);
				}else{
					surrogate_count+=1;
					write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);
				}	
		} else {
				if(output_file){
					fwrite(&(glyph -> bytes),sizeof(unsigned char),NON_SURROGATE_SIZE,output_file);
				}else{
					write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
				}	
		}
	}	
}

/* Parse the arguments*/
void parse_args(int argc,char** argv){
	int option_index, c;
	char* endian_convert = NULL;	
	option_index =0;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"UTF=value",required_argument,0,'u'},
		{0, 0, 0, 0}
	};

	/* If getopt() returns with a valid (its working correctly) 
	 * return code, then process the args! */
	
	while((c = getopt_long(argc, argv, "vhu:", long_options, &option_index)) != -1){

		switch(c){ 
			case 'v':
				verbosity += 1;

				break;
			case 'u':

				endian_convert = optarg;
				break;
			case 'h':
				print_help();
				break;	
			default:
				fprintf(stderr, "Unrecognized argument.\n");
				print_help();
				quit_converter(NO_FD);
				break;
		}

	}

	if(optind < argc){

		if(argc == optind+2){
			filename = strdup(argv[optind]);
			output_filename = strdup(argv[optind+1]);
		}
		else{
			filename = strdup(argv[optind]);
			output_filename = NULL;
		}
	} 
	else {
		fprintf(stderr, "Filename not given.\n");
		print_help();
		exit(EXIT_FAILURE);
	}


	if(endian_convert == NULL){
		fprintf(stderr, "Converson mode not given.\n");
		print_help();
		exit(EXIT_FAILURE);

	}

	if((strcmp(endian_convert, "16LE"))==0){ 

		conversion = LITTLE;
	} else if((strcmp(endian_convert, "16BE"))==0){

		conversion = BIG;
	} else {
		quit_converter(NO_FD);
		exit(EXIT_FAILURE);
	}


}

/* Print the help menu*/
void print_help() {
	int i=0;
	for(i=0;i<8;i++){
		printf("%s\n",USAGE[i]);
	}

	quit_converter(NO_FD);
}

/* Close all the file descripters and exit the program*/
void quit_converter(int fd){

	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(fd != NO_FD)
		close(fd);
	if(output_file)
		fclose(output_file);
}

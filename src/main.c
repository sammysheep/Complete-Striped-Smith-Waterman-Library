/*  main.c
 *  Created by Mengyao Zhao on 06/23/11.
 *	Version 0.1.4
 *  Last revision by Mengyao Zhao on 03/13/12.
 *	New features: make weight as options 
 */

#include <stdlib.h>
#include <stdint.h>
#include <emmintrin.h>
#include <zlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "ssw.h"
#include "kseq.h"

#ifdef __GNUC__
#define LIKELY(x) __builtin_expect((x),1)
#define UNLIKELY(x) __builtin_expect((x),0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

KSEQ_INIT(gzFile, gzread)

int8_t* char2num (char* seq, int8_t* table, int32_t l) {	// input l: 0; output l: length of the sequence
	int32_t i;
	int8_t* num = (int8_t*)calloc(l, sizeof(int8_t));
	for (i = 0; i < l; ++i) num[i] = table[(int)seq[i]];
	return num;
}

char* reverse_comple(const char* seq) {
	int32_t end = strlen(seq), start = 0;
	char* rc = (char*)calloc(end + 1, sizeof(char));
	int8_t rc_table[128] = {
		4, 4,  4, 4,  4,  4,  4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 4,  4, 4,  4,  4,  4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 4,  4, 4,  4,  4,  4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
		4, 4,  4, 4,  4,  4,  4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 84, 4, 71, 4,  4,  4, 67, 4, 4, 4, 4,  4, 4, 4, 4, 
		4, 4,  4, 4,  65, 65, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 84, 4, 71, 4,  4,  4, 67, 4, 4, 4, 4,  4, 4, 4, 4, 
		4, 4,  4, 4,  65, 65, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4 
	};
	rc[end] = '\0';
	-- end;				
	while (LIKELY(start < end)) {			
		rc[start] = (char)rc_table[(int8_t)seq[end]];		
		rc[end] = (char)rc_table[(int8_t)seq[start]];		
		++ start;					
		-- end;						
	}					
	if (start == end) rc[start] = (char)rc_table[(int8_t)seq[start]];			
	return rc;					
}							

void ssw_write (s_align* a, 
			kseq_t* ref_seq,
			char* read_name, 
			char* read_seq,	// strand == 0: original read; strand == 1: reverse complement read
			char* read_qual,	// strand == 0: original read base quality seq; strand == 1: reverse complement read quality seq
			int8_t* table, 
			int8_t strand,	// 0: forward aligned ; 1: reverse complement aligned 
			int8_t sam) {	// 0: Blast like output; 1: Sam format output

	if (sam == 0) {	// Blast like output
		fprintf(stdout, "target_name: %s\nquery_name: %s\noptimal_alignment_score: %d\t", ref_seq->name.s, read_name, a->score1);
		if (strand == 0) fprintf(stdout, "strand: +\t");
		else fprintf(stdout, "strand: -\t");
		if (a->ref_begin1) fprintf(stdout, "target_begin: %d\t", a->ref_begin1);
		fprintf(stdout, "target_end: %d\t", a->ref_end1);
		if (a->read_begin1) fprintf(stdout, "query_begin: %d\t", a->read_begin1);
		fprintf(stdout, "query_end: %d\n", a->read_end1);
		if (a->cigar) {
			int32_t i, c , q = a->ref_begin1 - 1, p = a->read_begin1 - 1;
			fprintf(stdout, "Target:\t");
			for (c = 0; c < a->cigarLen; ++c) {
				int32_t letter = 0xf&*(a->cigar + c);
				int32_t length = (0xfffffff0&*(a->cigar + c))>>4;
				if (letter == 1) for (i = 0; i < length; ++i) fprintf(stdout, "-");
				else {
					for (i = 0; i < length; ++i) fprintf(stdout, "%c", *(ref_seq->seq.s + q + i));
					q += length;
				}	
			}
			fprintf(stdout, "\n\t");
			q = a->ref_begin1 - 1;
			p = a->read_begin1 - 1;
			for (c = 0; c < a->cigarLen; ++c) {
				int32_t letter = 0xf&*(a->cigar + c);
				int32_t length = (0xfffffff0&*(a->cigar + c))>>4;
				if (letter == 0) {
					for (i = 0; i < length; ++i) 
						if (table[(int)*(ref_seq->seq.s + q + i)] == table[(int)*(read_seq + p + i)])fprintf(stdout, "|");
						else fprintf(stdout, "*");
					q += length;
					p += length;	
				}else { 
					for (i = 0; i < length; ++i) fprintf(stdout, "*");
					if (letter == 1) p += length;
					else q += length;
				}
			}
			fprintf(stdout, "\nQuery:\t");
			p = a->read_begin1 - 1;
			for (c = 0; c < a->cigarLen; ++c) {
				int32_t letter = 0xf&*(a->cigar + c);
				int32_t length = (0xfffffff0&*(a->cigar + c))>>4;
				if (letter == 2) for (i = 0; i < length; ++i) fprintf(stdout, "-");
				else {
					for (i = 0; i < length; ++i) fprintf(stdout, "%c", *(read_seq + p + i));
					p += length;
				}
			}	
		}
		fprintf(stdout, "\n\n");
	}else {	// Sam format output
		fprintf(stdout, "%s\t", read_name);
		if (a->score1 == 0) fprintf(stdout, "*\t0\t*\t*\t0\t0\t*\t*\n");
		else {
			fprintf(stdout, "%s\t%d\t", ref_seq->name.s, a->ref_begin1);
			int32_t c;
			for (c = 0; c < a->cigarLen; ++c) {
				int32_t letter = 0xf&*(a->cigar + c);
				int32_t length = (0xfffffff0&*(a->cigar + c))>>4;
				fprintf(stdout, "%d", length);
				if (letter == 0) fprintf(stdout, "M");
				else if (letter == 1) fprintf(stdout, "I");
				else fprintf(stdout, "D");
			}
			fprintf(stdout, "\t*\t0\t0\t");
			for (c = (a->read_begin1 - 1); c < a->read_end1; ++c) fprintf(stdout, "%c", read_seq[c]);
			fprintf(stdout, "\t");
			if (read_qual) fprintf(stdout, "%s\n", read_qual);
			else fprintf(stdout, "*\n");
		}
		//FIXME: print sam
	}
}

int main (int argc, char * const argv[]) {
	clock_t start, end;
	float cpu_time;
	gzFile read_fp, ref_fp;
	kseq_t *read_seq, *ref_seq;
	int32_t l, m, k, match = 2, mismatch = 2, gap_open = 3, gap_extension = 1, path = 0, reverse = 0, n = 5, sam = 0;
	int8_t* mat = (int8_t*)calloc(25, sizeof(int8_t));
	char mat_name[16];
	mat_name[0] = '\0';

	/* This table is used to transform amino acid letters into numbers. */
	int8_t aa_table[128] = {
		23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 
		23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 
		23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
		23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 
		23, 0,  20, 4,  3,  6,  13, 7,  8,  9,  23, 11, 10, 12, 2,  23, 
		14, 5,  1,  15, 16, 23, 19, 17, 22, 18, 21, 23, 23, 23, 23, 23, 
		23, 0,  20, 4,  3,  6,  13, 7,  8,  9,  23, 11, 10, 12, 2,  23, 
		14, 5,  1,  15, 16, 23, 19, 17, 22, 18, 21, 23, 23, 23, 23, 23 
	};

	/* This table is used to transform nucleotide letters into numbers. */
	int8_t nt_table[128] = {
		4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
		4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 4, 4, 4,  3, 0, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
		4, 4, 4, 4,  3, 0, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4 
	};
	
	int8_t* table = nt_table;

	// initialize scoring matrix for genome sequences
	for (l = k = 0; LIKELY(l < 4); ++l) {
		for (m = 0; LIKELY(m < 4); ++m) mat[k++] = l == m ? match : -mismatch;	/* weight_match : -weight_mismatch */
		mat[k++] = 0; // ambiguous base
	}
	for (m = 0; LIKELY(m < 5); ++m) mat[k++] = 0;

	// Parse command line.
	while ((l = getopt(argc, argv, "m:x:o:e:a:crs")) >= 0) {
		switch (l) {
			case 'm': match = atoi(optarg); break;
			case 'x': mismatch = atoi(optarg); break;
			case 'o': gap_open = atoi(optarg); break;
			case 'e': gap_extension = atoi(optarg); break;
			case 'a': strcpy(mat_name, optarg); break;
			case 'c': path = 1; break;
			case 'r': reverse = 1; break;
			case 's': sam = 1; break;
		}
	}
	if (optind + 2 > argc) {
		fprintf(stderr, "\n");
		fprintf(stderr, "Usage: ssw_test [options] ... <target.fa> <query.fa>\n");	
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "\t-m N\tN is a positive integer for weight match in genome sequence alignment.\n");
		fprintf(stderr, "\t-x N\tN is a positive integer. -N will be used as weight mismatch in genome sequence alignment.\n");
		fprintf(stderr, "\t-o N\tN is a positive integer. -N will be used as the weight for the gap opening.\n");
		fprintf(stderr, "\t-e N\tN is a positive integer. -N will be used as the weight for the gap extension.\n");
		fprintf(stderr, "\t-a FILE\tFor protein sequence alignment. FILE is either the Blosum or Pam weight matrix. Recommend to use the matrix\n\t\tincluding B Z X * columns. Otherwise, corresponding scores will be signed to 0.\n"); 
		fprintf(stderr, "\t-c\tReturn the alignment in cigar format.\n");
		fprintf(stderr, "\t-r\tThe best alignment will be picked between the original read alignment and the reverse complement read alignment.\n");
		fprintf(stderr, "\t-s\tOutput in SAM format.\n\n");
		return 1;
	}

	// Parse score matrix.
	if (strcmp(mat_name, "\0"))	{
		FILE *f_mat = fopen(mat_name, "r");
		char line[128];
		mat = (int8_t*)realloc(mat, 1024 * sizeof(int8_t));
		k = 0;
		while (fgets(line, 128, f_mat)) {
			if (line[0] == '*' || (line[0] >= 'A' && line[0] <= 'Z')) {
				char str[4], *s = str;
				str[0] = '\0';
				l = 1;
				while (line[l]) {
					if ((line[l] >= '0' && line[l] <= '9') || line[l] == '-') *s++ = line[l];	
					else if (str[0] != '\0') {					
						*s = '\0';
						mat[k++] = (int8_t)atoi(str);
						s = str;
						str[0] = '\0';			
					}
					++l;
				}
				if (str[0] != '\0') {
					*s = '\0';
					mat[k++] = (int8_t)atoi(str);
					s = str;
					str[0] = '\0';			
				}
			}
			m = k%24;
			while ((m%23 == 0 || m%22 == 0 || m%21 == 0 || m%20 == 0) && m != 0) {
				k++;
				m = k%24;
			} // If the weight matrix doesn't BZX*, set their values 0.
		}
		if (k == 0) {
			fprintf(stderr, "Problem of reading the weight matrix file.\n");
			return 1;
		} 
		fclose(f_mat);	
		if (mat[525] <= 0) {
			fprintf(stderr, "Improper weight matrix file format. Please use standard Blosum or Pam files.\n");
			return 1;
		}
		n = 24;
		table = aa_table;
	}

	read_fp = gzopen(argv[optind + 1], "r");
	read_seq = kseq_init(read_fp);
	if (sam) {
		fprintf(stdout, "@HD\tVN:1.4\tSO:queryname\n");
		while ((l = kseq_read(ref_seq)) >= 0) fprintf(stdout, "@SQ\tSN:%s\tLN:%d\n", ref_seq->name.s, (int32_t)ref_seq->seq.l);
	}
	start = clock();

	// alignment
	while ((m = kseq_read(read_seq)) >= 0) {
fprintf(stderr, "qual: %s\n", read_seq->qual.s);
		s_profile* p, *p_rc = 0;
		int32_t readLen = read_seq->seq.l; 
		char* read_rc = 0;
		int8_t* num, *num_rc = 0;
		
		num = char2num(read_seq->seq.s, table, readLen);
		p = ssw_init(num, readLen, mat, n, 2);
		if (reverse == 1 && n == 5) {
			read_rc = reverse_comple(read_seq->seq.s);
			num_rc = char2num(read_rc, table, readLen);
			p_rc = ssw_init(num_rc, readLen, mat, n, 2);
		}else if (reverse == 1 && n == 24) {
			fprintf (stderr, "Reverse complement alignment is not available for protein sequences. \n");
			return 1;
		}

		ref_fp = gzopen(argv[optind], "r");
		ref_seq = kseq_init(ref_fp);
		while ((l = kseq_read(ref_seq)) >= 0) {
			s_align* result, *result_rc = 0;
			int32_t refLen = ref_seq->seq.l;
			int8_t flag = 0;
			int8_t* ref_num = char2num(ref_seq->seq.s, table, refLen);
			if (path == 1) flag = 1;
			result = ssw_align (p, ref_num, refLen, gap_open, gap_extension, flag, 0);
			if (reverse == 1) result_rc = ssw_align(p_rc, ref_num, refLen, gap_open, gap_extension, flag, 0);
			if (result_rc && result_rc->score1 > result->score1) {
				if (sam) {
					int32_t length = result_rc->read_end1 - result_rc->read_begin1 + 1, s = 0, p = length - result_rc->read_begin1;
					char* qual_rc = (char*)calloc(length + 1, sizeof(char));
					qual_rc[length] = '\0';
				//	-- end;				
					while (LIKELY(s < length)) {			
						qual_rc[s] = read_seq->qual.s[p];		
					//	qual_rc[end] = (char)rc_table[(int8_t)seq[start]];		
						++ s;					
						-- p;						
					}					
					ssw_write (result_rc, ref_seq, read_seq->name.s, read_rc, qual_rc, table, 1, 1);
					free(qual_rc);
				}else ssw_write (result_rc, ref_seq, read_seq->name.s, read_rc, 0, table, 1, 0);
			}else if (result){
				if (sam) ssw_write(result, ref_seq, read_seq->name.s, read_seq->seq.s, read_seq->qual.s, table, 0, 1);
				else ssw_write(result, ref_seq, read_seq->name.s, read_seq->seq.s, 0, table, 0, 0);
			} else return 1;
			if (result_rc) align_destroy(result_rc);
			align_destroy(result);
			free(ref_num);
		}
		
		if(p_rc) init_destroy(p_rc);
		init_destroy(p);
		if (num_rc) free(num_rc);
		free(num);
		kseq_destroy(ref_seq);
		gzclose(ref_fp);
	}
	end = clock();
	cpu_time = ((float) (end - start)) / CLOCKS_PER_SEC;
	fprintf(stdout, "CPU time: %f seconds\n", cpu_time);

	free(mat);
	kseq_destroy(read_seq);
	gzclose(read_fp);
	return 0;
}

/*
This file is part of ftdock, a program for rigid-body protein-protein docking 
Copyright (C) 1997-2000 Gidon Moont

Biomolecular Modelling Laboratory
Imperial Cancer Research Fund
44 Lincoln's Inn Fields
London WC2A 3PX

+44 (0)20 7269 3348
http://www.bmm.icnet.uk/

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "rfftw.h"
#include "immintrin.h"




/************/

/* These values directly below may be altered, and the programs rebuilt */

#define MAX_ROTATIONS  100000
#define NUMBER_TO_KEEP 10000
#define NUMBER_OF_CONSTRAINTS 50
#define SAVED_HEADER_LINES 1000

/* I do not advise messing with anything below here */

/************/

/* Macros - really, you don't want to touch these! */

#define gaddress(x,y,z,grid_size) ( (z) + ( 2 * ( (grid_size) / 2 + 1 ) ) * ( (y) + (grid_size) * (x) ) )

#define gcentre(ordinate,grid_span,grid_size) ( (float)(ordinate) + .5 ) * (float)( (grid_span) / (float)(grid_size) ) - (float)( (grid_span) / 2 )

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define GENERAL_MEMORY_PROBLEM printf( "You do not have enough memory ([m|re]alloc failure)\nDying\n\n" ) ; exit( EXIT_FAILURE ) ;

#define pyth( x1 ,y1 ,z1 ,x2 ,y2 ,z2 ) (sqrt( ( ( x1 - x2 ) * ( x1 - x2 ) ) + ( ( y1 - y2 ) * ( y1 - y2 ) ) + ( ( z1 - z2 ) * ( z1 - z2 ) ) ) );

#define ADD(a,b) _mm256_add_ps(a,b)
#define AND(a,b) _mm256_and_ps(a,b)
#define ANDNOT(a,b) _mm256_andnot_ps (a,b)
#define CMPGE(a,b) _mm256_cmp_ps(a,b,_CMP_GE_OS)
#define CMPLE(a,b) _mm256_cmp_ps(a,b,_CMP_LE_OS)
#define DIV(a,b) _mm256_div_ps(a,b)
#define LOAD(p) _mm256_load_ps(p)
#define MAX(a,b) _mm256_max_ps(a,b)
#define MUL(a,b) _mm256_mul_ps(a,b)
#define OR(a,b) _mm256_or_ps(a,b)
#define SET1(a) _mm256_set1_ps(a)
#define SQRT(a) _mm256_sqrt_ps(a)
#define SUB(a,b) _mm256_sub_ps(a,b)

#define ELEMENTS_PER_ITERACIO 8

/************/

/* The structures comprising a Structure (representation of an organic molecule in 3D) */


struct Atom{
	int		serial ;
	char		atom_name[5] ;
	float	coord[4] ;
	float	occupancy ;
	float	temp_factor ;
	float	charge ;
} ;

struct Amino_Acid{	
	char			res_name[4] ;
	char			chainID[2] ;
	char			res_seq_plus_iCode[6] ;
	char			olc[2] ;
	int			nc ;
	int			size ;
	struct Atom	*Atom ;
} ;

struct Structure{
	char				ident[256] ;
	int				length ;
	struct Amino_Acid	*Residue ;	
} ;

/************/

/* Angles structure */

struct Angle{
	int	n ;
	int	*z_twist ;
	int	*theta ;
	int	*phi ;
} ;

/************/

/* Score structure */

struct Score{
	int		score ;
	int		coord[4] ;
     int		angle[4] ;
     float	rpscore ;
  	int		extra ;
} ;


/* Estructura auxiliar */

struct cache_line{

	//int elements_loaded;
	float x[ELEMENTS_PER_ITERACIO];
	float y[ELEMENTS_PER_ITERACIO];
	float z[ELEMENTS_PER_ITERACIO];
	float charge[ELEMENTS_PER_ITERACIO];
};


/************/

/* Matrix Structure */

struct Matrix{
	char		description[100] ;
	float	distance ;
	float	score[21][21] ;
} ;

/************/

/* Memory allocation sizes */


#define sizeof_Atom			sizeof( struct Atom )
#define sizeof_Amino_Acid	sizeof( struct Amino_Acid )
#define sizeof_Structure		sizeof( struct Structure )
#define SIZEOF_CACHE_LINE     sizeof( struct cache_line )



/************/

extern struct Structure read_pdb_to_structure( char *pdb_file_name ) ;
extern void write_structure_to_pdb( struct Structure This_Structure , char *pdb_file_name ) ;
extern struct Structure duplicate_structure( struct Structure This_Structure ) ;
extern struct Structure translate_structure( struct Structure This_Structure , float x_shift , float y_shift , float z_shift ) ;
extern struct Structure translate_structure_onto_origin( struct Structure This_Structure ) ;
extern struct Structure rotate_structure( struct Structure This_Structure , int z_twist , int theta , int phi ) ;
extern struct Structure merge_structures( struct Structure Structure_One , struct Structure Structure_Two ) ;
extern float radius_of_structure( struct Structure This_Structure ) ;
extern float total_span_of_structures( struct Structure Structure_1 , struct Structure Structure_2 ) ;

extern struct Angle generate_global_angles( int angle_step ) ;
extern struct Angle generate_range_of_angles( int angle_step , int angle_range , int z_twist , int theta , int phi ) ;

extern int gord( float position , float grid_span , int grid_size ) ;
extern float pythagoras( float x1 , float y1 , float z1 , float x2 , float y2 , float z2 ) ;

extern void discretise_structure( struct Structure This_Structure , float grid_span , int grid_size , fftw_real *grid ) ;
extern void surface_grid( float grid_span , int grid_size , fftw_real *grid , float surface , float internal_value ) ;

extern void assign_charges( struct Structure This_Structure ) ;
extern void electric_field( struct Structure This_Structure , float grid_span , int grid_size , fftw_real *grid ) ;
extern void electric_point_charge( struct Structure This_Structure , float grid_span , int grid_size , fftw_real *grid ) ;
extern void electric_field_zero_core( int grid_size , fftw_real *elec_grid , fftw_real *surface_grid , float internal_value ) ;

extern void qsort_scores( struct Score *Scores , int left , int right ) ;
extern void qsort_rpscores( struct Score *Scores , int left , int right ) ;

extern int numerical_sort( const void *a , const void *b ) ;








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


#include "structures.h"



void assign_charges( struct Structure This_Structure ) {

/************/

  /* Counters */

  int	residue , atom ;

/************/

  for( residue = 1 ; residue <= This_Structure.length ; residue ++ ) {
    for( atom = 1 ; atom <= This_Structure.Residue[residue].size ; atom ++ ) {

      This_Structure.Residue[residue].Atom[atom].charge = 0.0 ;

      /* peptide backbone */

      if( strcmp( This_Structure.Residue[residue].Atom[atom].atom_name , " N  " ) == 0 ) {
        if( strcmp( This_Structure.Residue[residue].res_name , "PRO" ) == 0 ) {
          This_Structure.Residue[residue].Atom[atom].charge = -0.10 ;
        } else {
          This_Structure.Residue[residue].Atom[atom].charge =  0.55 ;
          if( residue == 1 ) This_Structure.Residue[residue].Atom[atom].charge = 1.00 ;
        }
      }

      if( strcmp( This_Structure.Residue[residue].Atom[atom].atom_name , " O  " ) == 0 ) {
        This_Structure.Residue[residue].Atom[atom].charge = -0.55 ;
        if( residue == This_Structure.length  ) This_Structure.Residue[residue].Atom[atom].charge = -1.00 ;
      }

      /* charged residues */

      if( ( strcmp( This_Structure.Residue[residue].res_name , "ARG" ) == 0 ) && ( strncmp( This_Structure.Residue[residue].Atom[atom].atom_name , " NH" , 3 ) == 0 ) ) This_Structure.Residue[residue].Atom[atom].charge =  0.50 ;
      if( ( strcmp( This_Structure.Residue[residue].res_name , "ASP" ) == 0 ) && ( strncmp( This_Structure.Residue[residue].Atom[atom].atom_name , " OD" , 3 ) == 0 ) ) This_Structure.Residue[residue].Atom[atom].charge = -0.50 ;
      if( ( strcmp( This_Structure.Residue[residue].res_name , "GLU" ) == 0 ) && ( strncmp( This_Structure.Residue[residue].Atom[atom].atom_name , " OE" , 3 ) == 0 ) ) This_Structure.Residue[residue].Atom[atom].charge = -0.50 ;
      if( ( strcmp( This_Structure.Residue[residue].res_name , "Ly" ) == 0 ) && ( strcmp( This_Structure.Residue[residue].Atom[atom].atom_name , " NZ " ) == 0 ) ) This_Structure.Residue[residue].Atom[atom].charge =  1.00 ;

    }
  }

/************/

}



/************************/



void electric_field( struct Structure This_Structure , float grid_span , int grid_size , fftw_real *grid ) {


  /*Estructura que usaremos para copiar, de This_Structure, solo aquello que nos interesa y tan solo para los atomos que tienen carga != 0 */
   struct cache_line *cache;

  /* Counters */

   int	linies_ocupades = -1;
   int	residue , atom ;
   int 	charged_atoms = 0;
   int	i,j,k;

  /* Co-ordinates */

   int	x , y , z ;
   float	x_centre , y_centre , z_centre ;

  /* Variables */

   float	distance ;
   float	phi , epsilon ;
   int	num_iteracions;


    for( x = 0 ; x < grid_size ; x ++ ) {
      for( y = 0 ; y < grid_size ; y ++ ) {
        for( z = 0 ; z < grid_size ; z ++ ) {

          grid[gaddress(x,y,z,grid_size)] = (fftw_real)0 ;

        }
      }
    }

	for( residue = 1 ; residue <= This_Structure.length ; residue ++ ) {
	   for( atom = 1 ; atom <= This_Structure.Residue[residue].size ; atom ++ ) {
		  
		  if( This_Structure.Residue[residue].Atom[atom].charge != 0 ) {
		      charged_atoms++;
		  }
	   }
	}

    num_iteracions = charged_atoms&7 ? (charged_atoms>>3)+1 : charged_atoms>>3;

    if (posix_memalign((void**)&cache, 32, num_iteracions*SIZEOF_CACHE_LINE) != 0) {
        fprintf(stderr, "No memory.\n");
        exit(-1);
    }

    i = j = k = 0;

	for( residue = 1; residue <= This_Structure.length ; residue ++ ) {
	   for( atom = 1 ; atom <= This_Structure.Residue[residue].size ; atom ++ ) {
		  
		  if( This_Structure.Residue[residue].Atom[atom].charge != 0 ) {
		      
		      cache[i].x[j] = This_Structure.Residue[residue].Atom[atom].coord[1];
		      cache[i].y[j] = This_Structure.Residue[residue].Atom[atom].coord[2];
		      cache[i].z[j] = This_Structure.Residue[residue].Atom[atom].coord[3];
		      cache[i].charge[j] = This_Structure.Residue[residue].Atom[atom].charge;
		      
		      k++;
		      i = k/ELEMENTS_PER_ITERACIO;
		      j = k%ELEMENTS_PER_ITERACIO;
		  }
	   }
	}


	while (j != 0) {
	   
	   cache[i].x[j] = 0.0;
	   cache[i].y[j] = 0.0;
	   cache[i].z[j] = 0.0;
	   cache[i].charge[j] = 0.0;
	   
	   k++;
	   i = k/ELEMENTS_PER_ITERACIO;
	   j = k%ELEMENTS_PER_ITERACIO;
	}

 

  /*for( residue = 1 ; residue <= This_Structure.length ; residue ++ )
	total_atoms += This_Structure.Residue[residue].size;
  


  if (posix_memalign((void**)cache, 32, ( (total_atoms*sizeof(cache_line))/ELEMENTS_PER_ITERACIO ) != 0) {
    		fprintf(stderr, "No hay memoria.\n");
   	     exit(-1);
  }
  


  cache[0].elements_loaded = 0;

  for( residue = 1 ; residue <= This_Structure.length - (ELEMENTS_PER_ITERACIO + 1) ; residue ++ ) {
    for( atom = 1 ; atom <= This_Structure.Residue[residue].size ; atom ++ ) {

       if( This_Structure.Residue[residue].Atom[atom].charge != 0 ){

		cache[i].x[j] = This_Structure.Residue[residue].Atom[atom].coord[1];
		cache[i].y[j] = This_Structure.Residue[residue].Atom[atom].coord[2];
		cache[i].z[j] = This_Structure.Residue[residue].Atom[atom].coord[3];
		cache[i].charge  = This_Structure.Residue[residue].Atom[atom].charge;
	
		cache[i].elements_loaded++;
		j = (j+1) % ELEMENTS_PER_ITERACIO;

		if(j == 0){ 
			linies_ocupades = i;
			i++;
		     cache[i].elements_loaded = 0;
		}

	  }
    }
  }*/

  

  setvbuf( stdout , (char *)NULL , _IONBF , 0 ) ;

  printf( "  electric field calculations ( one dot / grid sheet ) " ) ;

  for( x = 0 ; x < grid_size ; x ++ ) {

    printf( "." ) ;

    x_centre  = gcentre( x , grid_span , grid_size ) ;

    for( y = 0 ; y < grid_size ; y ++ ) {

      y_centre  = gcentre( y , grid_span , grid_size ) ;

      for( z = 0 ; z < grid_size ; z ++ ) {

        z_centre  = gcentre( z , grid_span , grid_size ) ;

        phi = 0 ;

	   for (i=0; i < num_iteracions; i++) {
                    
                    // computing distances (inlining pythagoras)
                    __m256 xaux, yaux, zaux;
                    
                    xaux = LOAD(&cache[i].x[0]);
                    yaux = LOAD(&cache[i].y[0]);
                    zaux = LOAD(&cache[i].z[0]);
                    
                    xaux = SUB(xaux, SET1(x_centre));
                    yaux = SUB(yaux, SET1(y_centre));
                    zaux = SUB(zaux, SET1(z_centre));
                    
                    xaux = MUL(xaux, xaux);
                    yaux = MUL(yaux, yaux);
                    zaux = MUL(zaux, zaux);
                    
                    __m256 distances = SQRT(ADD(ADD(xaux, yaux), zaux));
                    distances = MAX(distances, SET1(2.0));
                    
                    // computing epsilons
                    __m256 epsilon80, epsilon4, epsilon, mask80, mask4, mask;
                    
                    mask80 	= CMPGE(distances, SET1(8.0));
                    epsilon80 = AND(mask80, SET1(80));
                    
                    mask4    = CMPLE(distances, SET1(6.0));
                    epsilon4 = AND(mask4, SET1(4));
                    
                    mask =    OR(mask80, mask4);
                    epsilon = MUL(distances, SET1(38));
                    epsilon = SUB(epsilon, SET1(224));
                    epsilon = ANDNOT(mask, epsilon);
                    
                    __m256 epsilons = OR(OR(epsilon80, epsilon4), epsilon);
                    
                    // computing phis
                    __m256 phis = MUL(epsilons, distances);
                    phis = DIV(LOAD(&cache[i].charge[0]), phis);
                    
                    phi += (phis[0] + phis[1] + phis[2] + phis[3] + phis[4] + phis[5] + phis[6] + phis[7]) ;
                    
        }

        grid[gaddress(x,y,z,grid_size)] = (fftw_real)phi ;

      }
    }
  }

  printf( "\n" ) ;

/************/

  return ;

}



/************************/



void electric_point_charge( struct Structure This_Structure , float grid_span , int grid_size , fftw_real *grid ) {

/************/

  /* Counters */

  int	residue , atom ;

  /* Co-ordinates */

  int	x , y , z ;
  int	x_low , x_high , y_low , y_high , z_low , z_high ;

  float		a , b , c ;
  float		x_corner , y_corner , z_corner ;
  float		w ;

  /* Variables */

  float		one_span ;

/************/

  for( x = 0 ; x < grid_size ; x ++ ) {
    for( y = 0 ; y < grid_size ; y ++ ) {
      for( z = 0 ; z < grid_size ; z ++ ) {

        grid[gaddress(x,y,z,grid_size)] = (fftw_real)0 ;

      }
    }
  }

/************/

  one_span = grid_span / (float)grid_size ;

  for( residue = 1 ; residue <= This_Structure.length ; residue ++ ) {
    for( atom = 1 ; atom <= This_Structure.Residue[residue].size ; atom ++ ) {

      if( This_Structure.Residue[residue].Atom[atom].charge != 0 ) {

        x_low = gord( This_Structure.Residue[residue].Atom[atom].coord[1] - ( one_span / 2 ) , grid_span , grid_size ) ;
        y_low = gord( This_Structure.Residue[residue].Atom[atom].coord[2] - ( one_span / 2 ) , grid_span , grid_size ) ;
        z_low = gord( This_Structure.Residue[residue].Atom[atom].coord[3] - ( one_span / 2 ) , grid_span , grid_size ) ;

        x_high = x_low + 1 ;
        y_high = y_low + 1 ;
        z_high = z_low + 1 ;

        a = This_Structure.Residue[residue].Atom[atom].coord[1] - gcentre( x_low , grid_span , grid_size ) - ( one_span / 2 ) ;
        b = This_Structure.Residue[residue].Atom[atom].coord[2] - gcentre( y_low , grid_span , grid_size ) - ( one_span / 2 ) ;
        c = This_Structure.Residue[residue].Atom[atom].coord[3] - gcentre( z_low , grid_span , grid_size ) - ( one_span / 2 ) ;

        for( x = x_low ; x <= x_high  ; x ++ ) {
 
          x_corner = one_span * ( (float)( x - x_high ) + .5 ) ;

          for( y = y_low ; y <= y_high  ; y ++ ) {

            y_corner = one_span * ( (float)( y - y_high ) + .5 ) ;

            for( z = z_low ; z <= z_high  ; z ++ ) {

              z_corner = one_span * ( (float)( z - z_high ) + .5 ) ;

              w = ( ( x_corner + a ) * ( y_corner + b ) * ( z_corner + c ) ) / ( 8.0 * x_corner * y_corner * z_corner ) ;

              grid[gaddress(x,y,z,grid_size)] += (fftw_real)( w * This_Structure.Residue[residue].Atom[atom].charge ) ;

            }
          }
        }

      }

    }
  }

/************/

  return ;

}



/************************/



void electric_field_zero_core( int grid_size , fftw_real *elec_grid , fftw_real *surface_grid , float internal_value ) {

/************/

  /* Co-ordinates */

  int	x , y , z ;

/************/

  for( x = 0 ; x < grid_size ; x ++ ) {
    for( y = 0 ; y < grid_size ; y ++ ) {
      for( z = 0 ; z < grid_size ; z ++ ) {

        if( surface_grid[gaddress(x,y,z,grid_size)] == (fftw_real)internal_value ) elec_grid[gaddress(x,y,z,grid_size)] = (fftw_real)0 ;

      }
    }
  }

/************/

  return ;

}

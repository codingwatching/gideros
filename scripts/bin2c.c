/*
 * bin2c.c
 *
 *  Created on: 12 juin 2015
 *      Author: Nico
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#ifndef O_BINARY
#define O_BINARY	0
#endif

/* Replace . with _ */
char*
make_ident ( char* name )
{
	char* ret;
	char* p;

	ret = strdup ( name );

	for ( p = ret; p[0]; p++ )
	{
		if ( !isalnum ( p[0] ) ) p[0] = '_';
	}
	return ret;
}

int
main ( int argc, char* argv[] )
{
	unsigned char buf[BUFSIZ];
	char* ident;
	int fd, i, total, rd, need_comma;

	if ( argc < 2 )
	{
		fprintf ( stderr, "Usage: %s binary_file > output_file\n", argv[0] );
		return -1;
	}

	fd = open ( argv[1], O_BINARY | O_RDONLY );
	if ( fd == -1 )
	{
		fprintf ( stderr, "%s: can't open %s for reading\n", argv[0], argv[1] );
		return -1;
	}

	ident = make_ident ( argv[1] );

	printf ( "const unsigned char %s[] = {", ident );
	for ( total = 0, need_comma = 0; ( rd = read ( fd, buf, BUFSIZ ) ) != 0; )
	{
		if ( rd == -1 )
		{
			fprintf ( stderr, "%s: file read error\n", argv[0] );
			return -1;
		}
		for ( i = 0; i < rd; i++ )
		{
			if ( need_comma ) printf ( ", " );
			else need_comma = 1;
			if ( ( total % 11 ) == 0 ) printf ( "\n\t" );
			printf ( "0x%.2x", buf[i] );
			total++;
		}
	}
	printf ( "\n};\n" );

	close ( fd );
	free ( ident );

	return 0;
}


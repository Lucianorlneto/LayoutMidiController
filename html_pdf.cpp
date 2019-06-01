/* -*- mode: c++; tab-width: 4; indent-tabs-mode: t; eval: (progn (c-set-style "stroustrup") (c-set-offset 'innamespace 0)); -*-
 * vi:set ts=4 sts=4 sw=4 noet :
 *
 * Copyright 2010, 2011 wkhtmltopdf authors
 *
 * This file is part of wkhtmltopdf.
 *
 * wkhtmltopdf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * wkhtmltopdf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with wkhtmltopdf.  If not, see <http: *www.gnu.org/licenses/>.
 */

/* This is a simple example program showing how to use the wkhtmltopdf c bindings */
#include <stdbool.h>
#include <stdio.h>
#include <wkhtmltox/pdf.h>
#include <fstream>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sstream>

using namespace std;

int arg1, arg2;

/* Print out loading progress information */
void progress_changed(wkhtmltopdf_converter * c, int p) {
	printf("%3d%%\r",p);
	fflush(stdout);
}

/* Print loading phase information */
void phase_changed(wkhtmltopdf_converter * c) {
	int phase = wkhtmltopdf_current_phase(c);
	printf("%s\n", wkhtmltopdf_phase_description(c, phase));
}

/* Print a message to stderr when an error occurs */
void error(wkhtmltopdf_converter * c, const char * msg) {
	fprintf(stderr, "Error: %s\n", msg);
}

/* Print a message to stderr when a warning is issued */
void warning(wkhtmltopdf_converter * c, const char * msg) {
	fprintf(stderr, "Warning: %s\n", msg);
}

void gera_pdf(){
	wkhtmltopdf_global_settings * gs;
	wkhtmltopdf_object_settings * os;
	wkhtmltopdf_converter * c;

	/* Init wkhtmltopdf in graphics less mode */
	wkhtmltopdf_init(false);

	/*
	 * Create a global settings object used to store options that are not
	 * related to input objects, note that control of this object is parsed to
	 * the converter later, which is then responsible for freeing it
	 */
	gs = wkhtmltopdf_create_global_settings();
	/* We want the result to be storred in the file called test.pdf */
	wkhtmltopdf_set_global_setting(gs, "out", "teste.pdf");

	wkhtmltopdf_set_global_setting(gs, "load.cookieJar", "myjar.jar");
	/*
	 * Create a input object settings object that is used to store settings
	 * related to a input object, note again that control of this object is parsed to
	 * the converter later, which is then responsible for freeing it
	 */
	os = wkhtmltopdf_create_object_settings();
	/* We want to convert to convert the qstring documentation page */
	wkhtmltopdf_set_object_setting(os, "page", "folha_criada.html");

	/* Create the actual converter object used to convert the pages */
	c = wkhtmltopdf_create_converter(gs);

	/* Call the progress_changed function when progress changes */
	wkhtmltopdf_set_progress_changed_callback(c, progress_changed);

	/* Call the phase _changed function when the phase changes */
	wkhtmltopdf_set_phase_changed_callback(c, phase_changed);

	/* Call the error function when an error occurs */
	wkhtmltopdf_set_error_callback(c, error);

	/* Call the warning function when a warning is issued */
	wkhtmltopdf_set_warning_callback(c, warning);

	/*
	 * Add the the settings object describing the qstring documentation page
	 * to the list of pages to convert. Objects are converted in the order in which
	 * they are added
	 */
	wkhtmltopdf_add_object(c, os, NULL);

	/* Perform the actual conversion */
	if (!wkhtmltopdf_convert(c))
		fprintf(stderr, "Conversion failed!");

	/* Output possible http error code encountered */
	printf("httpErrorCode: %d\n", wkhtmltopdf_http_error_code(c));

	/* Destroy the converter object since we are done with it */
	wkhtmltopdf_destroy_converter(c);

	/* We will no longer be needing wkhtmltopdf funcionality */
	wkhtmltopdf_deinit();
}

void gera_html(int linhas, string result){
	ofstream myfile;
	myfile.open ("folha_criada.html");             
	myfile <<
	"<!DOCTYPE html>\n" <<
	"<html>\n" <<
	"<head>\n" <<
	"<title></title>\n" <<
	"<script type='text/javascript' src='jquery.min.js'></script> \n" <<
	"<script type='text/javascript' src='jquery-barcode.js'></script>  \n" <<
	"<style type='text/css'>\n" <<
	".page{\n" <<
	"width: 595pt;\n" <<
	"height: 842pt;\n" <<
	"}\n" <<
	"table{\n" <<
	"width: 100%;\n" <<
	"border-collapse: collapse;\n" <<
	"border-spacing: 0;\n" <<
	"}\n" <<
	"th{\n" <<
	"border: solid black 1px;\n" <<
	"height: 50px;\n" <<
	"width: 50px;\n" <<
	"border-top: none;\n" <<
	"}\n" <<
	"td{\n" <<
	"border: solid black 1px;\n" <<
	"}\n" <<
	"body{\n" <<
	"margin: 0;\n" <<
	"padding: 0;\n" <<
	"height: 1305px;\n" <<
	"}\n" <<
	".rotate{\n" <<
	    "-moz-transform: rotate(90.0deg);  /* FF3.5+ */\n" <<
	    "-o-transform: rotate(90.0deg);  /* Opera 10.5 */\n" <<
	    "-webkit-transform: rotate(90.0deg);  /* Saf3.1+, Chrome */\n" <<
	    "filter:  progid:DXImageTransform.Microsoft.BasicImage(rotation=0.083);  /* IE6,IE7 */\n" <<
	    "-ms-filter: 'progid:DXImageTransform.Microsoft.BasicImage(rotation=0.083)'; /* IE8 */\n" <<
	".rotateimg180 {\n" <<
	"-webkit-transform:rotate(180deg);\n" <<
	"-moz-transform: rotate(180deg);\n" <<
	"-ms-transform: rotate(180deg);\n" <<
	"-o-transform: rotate(180deg);\n" <<
	"transform: rotate(180deg);\n" <<
	"}\n" <<
	".rotateimg270 {\n" <<
	"-webkit-transform:rotate(270deg);\n" <<
	"-moz-transform: rotate(270deg);\n" <<
	"-ms-transform: rotate(270deg);\n" <<
	"-o-transform: rotate(270deg);\n" <<
	"transform: rotate(270deg);\n" <<
	"}\n" <<
	"margin-left: -10em;\n" <<
	"margin-right: -10em;\n" <<
	"}\n" <<
	"</style>\n" <<
	"</head>\n" <<
	"\n" <<
	"<body>\n" <<
	"\n" <<
	"<div style='left: 26px; top: 62px; position: absolute'><img src='markers/4x4_1000-3.png' heigh='100px' width='100px' class='rotate'></div>\n" <<
	"<div style='right: 126px; top: 62px; position: absolute'><img src='markers/4x4_1000-1.png' heigh='100px' width='100px' class='rotate'></div>\n" <<
	"<div style='left: 26px; bottom: 54px; position: absolute'><img src='markers/4x4_1000-4.png' heigh='100px' width='100px' class='rotate'></div>\n" <<
	"<div style='right: 126px; bottom: 54px; position: absolute'><img src='markers/4x4_1000-2.png' heigh='100px' width='100px' class='rotate'></div> \n" <<
	"\n" <<
	"<div class='rotate' style='position: absolute; top: 600px; right: 0px;' id='barcode'></div>\n" <<
	"\n" <<
	"<div style='position: absolute; top: 10px; left: 125px; right: 125px; bottom: 73px;'>\n" <<
	"<table align='center' width='646px' style='height: 1237px'>\n" <<
	"<thead>\n" <<
	"<tr>\n" <<
	"<th class='rotate'>C</th>\n" <<
	"<th class='rotate'>C#</th>\n" <<
	"<th class='rotate'>D</th>\n" <<
	"<th class='rotate'>D#</th>\n" <<
	"<th class='rotate'>E</th>\n" <<
	"<th class='rotate'>F</th>\n" <<
	"<th class='rotate'>F#</th>\n" <<
	"<th class='rotate'>G</th>\n" <<
	"<th class='rotate'>G#</th>\n" <<
	"<th class='rotate'>A</th>\n" <<
	"<th class='rotate'>A#</th>\n" <<
	"<th class='rotate'>B</th>\n" <<
	"<th class='rotate'>C</th>\n" <<
	"</tr>\n" <<
	"</thead>\n" <<
	"<tbody>\n";
	for (int i = 0; i < linhas; ++i)
	{
		myfile <<
		"<tr>\n";
		if((i+1)%arg2 == 0){
			myfile <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n" <<
			"<td style='border-top: none; border-bottom: solid gray 1px'></td>\n";
		}else{
			myfile <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n" <<
			"<td></td>\n";
		}
		myfile <<
		"</tr>\n";
	}
	myfile << 
	"</tobdy>\n" <<
	"</table>\n" <<
	"</div>\n" <<
	"</body>\n";

	myfile <<
	"<script type='text/javascript'>\n" <<
	"$('#barcode').barcode('" << result << "', 'ean8', {\n" <<
	"showHRI: false,\n" <<
	"barWidth: 5,\n" <<
	"});\n" <<
	"</script>\n";

	myfile.close();
}

/* Main method convert pdf */
int main(int argc, char** argv) {
	
	arg1 = atoi(argv[1]);

	arg2 = atoi(argv[2]);

	int linhas = arg1*arg2;

	string result;          // string which will contain the result

	ostringstream convert;   // stream used for the conversion

	if(linhas < 10){
		convert << "0" << linhas;
	}else{
		convert << linhas;
	}

	if(arg1 < 10){
		convert << "0" << arg1;
	}else{
		convert << arg1;
	}

	if(arg2 < 10){
		convert << "0" << arg2;
	}else{
		convert << arg2;
	}

	result = convert.str();

	// cout << result.size() << endl;

	while(result.size() < 8){
		result.append("0");
	};

	// cout << result << endl;

	gera_html(linhas, result);

	gera_pdf();

	return 0;
}

/*******************************************************************************
        ssl_authentication.c
        Z-World, 2003

        This program provides three users, "foo", "foo2", and "foo3",
        for whom the passwords are "bar", "bar2", and "bar3",
        respectively.  Each of the three users can be toggled
        between being valid and invalid by pressing '1', '2', or '3'
        on the keyboard, respectively.

        Authentication modes can be set among basic, digest, and no
        authentication by pressing the 'b', 'd', or 'n' keys,
        respectively.

        This sample is a modification of authentication.c (in the HTTP
        samples folder) to do HTTP authentication over HTTPS, secured
        by SSL. See the comments in authentication.c for more information
        on HTTP authentication. Note that SSL encrypts both the password
        and the web page.

        ***NOTE*** This sample will NOT compile without first creating a
        certificate with the name and path specified in the #ximport line
        ("#ximport 'cert\mycert.dcc' SSL_CERTIFICATE") below. See the SSL
        Walkthrough, Section 4.1 in the SSL User's Manual for information
        on creating certificates for SSL programs.

        Most modern browsers support digest authentication, including
        the most recent versions of Internet Explorer, Netscape/Mozilla,
        and Opera.  Netscape and Mozilla ignore the "stale" flag in
        digest authentication, and hence will sometimes ask for the
        username/password again when it is not strictly necessary.
        Also note that Netscape 4.x does NOT support digest
        authentication.

*******************************************************************************/

/***********************************
 * Configuration                   *
 * -------------                   *
 * The following fields may need   *
 * to be altered to match your     *
 * preferences.                    *
 ***********************************/

/*
 * Pick the predefined TCP/IP configuration for this sample.  See
 * LIB\TCPIP\TCP_CONFIG.LIB for instructions on how to set the
 * configuration.
 */
#define TCPCONFIG 					1

/*
 * HTTP_MAXBUFFER is used for each web server to hold received and
 * transferred information.  In particular, it holds each header line
 * received from the web client.  Digest authentication generally
 * results in a large WWW-Authenticate header that can be larger than
 * the default HTTP_MAXBUFFER value of 256.  512 bytes should be
 * sufficient.
 */
#define HTTP_MAXBUFFER				512

/*
 * By default, digest authentication is turned off.  Note that you
 * can set USE_HTTP_BASIC_AUTHENTICATION to 0 to remove the code for
 * basic authentication at compile time.
 */
#define USE_HTTP_DIGEST_AUTHENTICATION	1

/*
 * If you want to associate multiple users with a particular
 * resource (web page), then you need to set the following macro.
 * This macro defines how many users can be associated with a web
 * page, and defaults to 1.
 */
#define SSPEC_USERSPERRESOURCE 	3

/*
 * If you are using only the ZSERVER.LIB functions to manage the table
 * of HTTP pages, then you can disable code support for the
 * http_flashspec[] array.
 */
#define HTTP_NO_FLASHSPEC

/*
 *  memmap forces the code into xmem.  Since the typical stack is larger
 *  than the root memory, this is commonly a desirable setting.  Another
 *  option is to do #memmap anymem 8096 which will force code to xmem when
 *  the compiler notices that it is generating within 8096 bytes of the
 *  end.
 *
 *  #use the Dynamic C TCP/IP stack library and the HTTP application library
 */

// This macro determines the number of HTTP servers that will use
// SSL (HTTPS servers). In this case, we have 2 total servers, and
// this defines one of them to be HTTPS
#define HTTP_SSL_SOCKETS 1

// SSL Stuff
// This macro tells the HTTP library to use SSL
#define USE_HTTP_SSL

// Import the certificate (use default of SSL_CERTIFICATE, used automatically
// by the HTTP library, no API calls necessary)
#ximport "cert\mycert.dcc" SSL_CERTIFICATE

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"

/*
 *  ximport is a Dynamic C language feature that takes the binary image
 *  of a file, places it in extended memory on the controller, and
 *  associates a symbol with the physical address on the controller of
 *  the image.
 */
#ximport "samples/tcpip/http/pages/static.html"		index_html
#ximport "samples/tcpip/http/pages/rabbit1.gif"		rabbit1_gif

/*
 *  http_types gives the HTTP server hints about handling incoming
 *  requests.  The server compares the extension of the incoming
 *  request with the http_types list and returns the second field
 *  as the Content-Type field.  The third field defines a custom
 *  function to handle that mime type.
 */
const HttpType http_types[] =
{
   { ".html", "text/html", NULL},
   { ".gif", "image/gif", NULL}
};

void main(void)
{
	int user1;
	int user2;
	int user3;
	int user1_enabled;
	int user2_enabled;
	int user3_enabled;
	int page1;
	int page2;
	int ch;

	printf("Press '1', '2', or '3' to disable/enable the three users.\n");
	printf("Press 'b', 'd', or 'n' to set the authentication to basic, digest, or none.\n\n");

	/*
	 *  sock_init initializes the TCP/IP stack.
	 *  http_init initializes the web server.
	 */
	sock_init();
	http_init();

	/*
	 * HTTP_DIGEST_AUTH is the default authentication type when
	 * digest authentication has been enabled, so this line is not
	 * strictly necessary.  The other possible values are
	 * HTTP_BASIC_AUTH and HTTP_NO_AUTH.
	 */
	http_setauthentication(HTTP_DIGEST_AUTH);
	printf("Using digest authentication\n");

	/*
	 * The following lines add three users, a web page, and an image, and
	 * associates the users with the web page.  The userx_enabled
	 * variables are used to keep track of which users are current
	 * enabled.
	 */
	user1_enabled = 1;
	user2_enabled = 1;
	user3_enabled = 1;
	user1 = sauth_adduser("foo", "bar", SERVER_HTTPS);
	user2 = sauth_adduser("foo2", "bar2", SERVER_HTTPS);
	user3 = sauth_adduser("foo3", "bar3", SERVER_HTTPS);

	page1 = sspec_addxmemfile("/", index_html, SERVER_HTTPS);
	sspec_adduser(page1, user1);
	sspec_adduser(page1, user2);
	sspec_adduser(page1, user3);
	sspec_setrealm(page1, "Admin");

	page2 = sspec_addxmemfile("index.html", index_html, SERVER_HTTPS);
	sspec_adduser(page2, user1);
	sspec_adduser(page2, user2);
	sspec_adduser(page2, user3);
	sspec_setrealm(page2, "Admin");

	sspec_addxmemfile("rabbit1.gif", rabbit1_gif, SERVER_HTTPS);

	/*
	 *  tcp_reserveport causes the web server to ignore requests when there
	 *  isn't an available socket (HTTP_MAXSERVERS are all serving index_html
	 *  or rabbit1.gif).  This saves some memory, but can cause the client
	 *  delays when retrieving pages (versus increasing HTTP_MAXSERVERS).
    *  We reserve port 443 for HTTPS communication
	 */
	tcp_reserveport(443);

	while (1) {
		/*
		 * Watch for user keypresses
		 */
		if (kbhit()) {
			ch = getchar();
			switch (ch) {
			case '1':
				/*
				 * Handle the keypress for User 1
				 */
				user1_enabled = !user1_enabled;
				if (user1_enabled) {
					/*
					 * sspec_adduser() adds a user to a resource
					 */
					sspec_adduser(page1, user1);
					sspec_adduser(page2, user1);
					printf("User 1 enabled\n");
				}
				else {
					/*
					 * sspec_removeuser() removes a user from a resource
					 */
					sspec_removeuser(page1, user1);
					sspec_removeuser(page2, user1);
					printf("User 1 disabled\n");
				}
				break;
			case '2':
				user2_enabled = !user2_enabled;
				if (user2_enabled) {
					sspec_adduser(page1, user2);
					sspec_adduser(page2, user2);
					printf("User 2 enabled\n");
				}
				else {
					sspec_removeuser(page1, user2);
					sspec_removeuser(page2, user2);
					printf("User 2 disabled\n");
				}
				break;
			case '3':
				user3_enabled = !user3_enabled;
				if (user3_enabled) {
					sspec_adduser(page1, user3);
					sspec_adduser(page2, user3);
					printf("User 3 enabled\n");
				}
				else {
					sspec_removeuser(page1, user3);
					sspec_removeuser(page2, user3);
					printf("User 3 disabled\n");
				}
				break;
			case 'b':
				http_setauthentication(HTTP_BASIC_AUTH);
				printf("Using basic authentication\n");
				break;
			case 'd':
				http_setauthentication(HTTP_DIGEST_AUTH);
				printf("Using digest authentication\n");
				break;
			case 'n':
				http_setauthentication(HTTP_NO_AUTH);
				printf("Using no authentication\n");
				break;
			}
		}

		/*
		 * http_handler needs to be called to handle the active http servers
		 */
		http_handler();
	}
}
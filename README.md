libneo4j-client
===============


About
-----

libneo4j-client is a client library and command line shell for Neo4j. The
client library is written in C, and is intended as a foundation on which basic
tools and drivers for various languages may be built. libneo4j-client takes
care of all the detail of establishing a session with a Neo4j server, sending
statements for evaluation, and retrieving results.

neo4j-client, the command line shell, is included with the libneo4j-client
distribution and uses libneo4j-client for all interaction with Neo4j server.

For more details, see [the project page](
https://cleishm.github.io/libneo4j-client/) and the [FAQ](
https://github.com/cleishm/libneo4j-client/wiki/FAQ).


Requirements
------------

libneo4j-client is known to work on GNU/Linux, Mac OS X and FreeBSD. It
requires neo4j 3.0.0-M02 or later.

Note that libneo4j-client is still an alpha release, and may change in
incompatible ways before a stable release is made.


Getting Started
---------------

If you're using Mac OS X, libneo4j-client can be installed using homebrew:

```console
$ brew install cleishm/neo4j/libneo4j-client
```

If you're using Ubuntu, neo4j-client can be install using APT:

```console
$ sudo add-apt-repository ppa:cleishm/libneo4j-client
$ sudo apt-get update
$ sudo apt-get install neo4j-client libneo4j-client-dev
```

There are also packages available for other platforms, including
[Debian](https://packages.debian.org/source/sid/libneo4j-client),
[Ubuntu](https://launchpad.net/~cleishm/+archive/ubuntu/libneo4j-client),
[Fedora](https://build.opensuse.org/package/binaries/home:cleishm/libneo4j-client?repository=Fedora_23),
[CentOS](https://build.opensuse.org/package/binaries/home:cleishm/libneo4j-client?repository=CentOS_7) and
[openSUSE](https://build.opensuse.org/package/binaries/home:cleishm/libneo4j-client?repository=openSUSE_Tumbleweed).

Otherwise, please see [Building](#building) below.


neo4j-client Usage
------------------

neo4j-client is a command shell for Neo4j. It supports secure connections
to Neo4j server, sending of statements (including multiline statements),
persistent command history, and rendering of results to tables or CSV.

Basic usage:

```console
$ neo4j-client -v neo4j://user:pass@localhost:7687
The authenticity of host 'localhost:7687' could not be established.
TLS certificate fingerprint is ded0fd2e893cd0b579f47f7798e10cb68dfa2fd3bc9b3c973157da81bab451d74f9452ba99a9c5f66dadb8a360959e5ebd8abb2d7c81230841e60531a96d268.
Would you like to trust this host (NO/yes/once)? yes
neo4j> :help
:quit                  Exit the shell
:connect '<url>'       Connect to the specified URL
:disconnect            Disconnect the client from the server
:help                  Show usage information
:output (table|csv)    Set the output format
:width <n>             Set the number of columns in the table output
neo4j>
neo4j>
neo4j> MATCH (n:Person) RETURN n LIMIT 3;
+----------------------------------------------------------------------------+
| n                                                                          |
+----------------------------------------------------------------------------+
| (:Person{born:1964,name:"Keanu Reeves"})                                   |
| (:Person{born:1967,name:"Carrie-Anne Moss"})                               |
| (:Person{born:1961,name:"Laurence Fishburne"})                             |
+----------------------------------------------------------------------------+
neo4j> :exit
$
```


libneo4j-client Usage
---------------------

libneo4j-client provides a single C header file, `neo4j-client.h`, for
inclusion in source code using the libneo4j-client API. The API is described in
the [API Documentation](#api_documentation).

libneo4j-client can be included in your project by linking the library at
compile time, typically using the linking flags
`-lneo4j-client -lssl -lcrypto -lm`.  Alternatively, libneo4j-client ships with
a [pkg-config]( https://wiki.freedesktop.org/www/Software/pkg-config/)
description file, enabling you to obtain the required flags using
`pkg-config --libs libneo4j-client`.


API Documentation
-----------------

API documentation for the latest release is available at
[https://cleishm.github.io/libneo4j-client/doc/latest/neo4j-client\_8h.html](
[https://cleishm.github.io/libneo4j-client/doc/latest/neo4j-client_8h.html).

Documentation can be built using `make doc`, which will use doxygen to generate
documentation and output it into the `doc/` directory of the libneo4j-client
source tree. See [Building](#building) below.


Example
-------

```C
#include <neo4j-client.h>
#include <errno.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    neo4j_client_init();

    /* use NEO4J_INSECURE when connecting to disable TLS */
    neo4j_connection_t *connection =
            neo4j_connect("neo4j://localhost:7687", NULL, NEO4J_INSECURE);
    if (connection == NULL)
    {
        neo4j_perror(stderr, errno, "Connection failed");
        return EXIT_FAILURE;
    }

    neo4j_session_t *session = neo4j_new_session(connection);
    if (session == NULL)
    {
        neo4j_perror(stderr, errno, "Failed to start session");
        return EXIT_FAILURE;
    }

    neo4j_result_stream_t *results =
            neo4j_run(session, "RETURN 'hello world'", neo4j_null);
    if (results == NULL)
    {
        neo4j_perror(stderr, errno, "Failed to run statement");
        return EXIT_FAILURE;
    }

    neo4j_result_t *result = neo4j_fetch_next(results);
    if (results == NULL)
    {
        neo4j_perror(stderr, errno, "Failed to fetch result");
        return EXIT_FAILURE;
    }

    neo4j_value_t value = neo4j_result_field(result, 0);
    char buf[128];
    printf("%s\n", neo4j_tostring(value, buf, sizeof(buf)));

    neo4j_close_results(results);
    neo4j_end_session(session);
    neo4j_close(connection);
    neo4j_client_cleanup();
    return EXIT_SUCCESS;
}
```


Building
--------

To build software using libneo4j-client, consider installing libneo4j-client
using the package management system for your operating system (currently
Mac OS X,
[Debian](https://packages.debian.org/source/sid/libneo4j-client),
[Ubuntu](https://launchpad.net/~cleishm/+archive/ubuntu/libneo4j-client),
[Fedora](https://build.opensuse.org/package/binaries/home:cleishm/libneo4j-client?repository=Fedora_23),
[CentOS](https://build.opensuse.org/package/binaries/home:cleishm/libneo4j-client?repository=CentOS_7) and
[openSUSE](https://build.opensuse.org/package/binaries/home:cleishm/libneo4j-client?repository=openSUSE_Tumbleweed)).

If libneo4j-client is not available via your package management system,
please [download the latest release](
https://github.com/cleishm/libneo4j-client/releases), unpack and then:

```console
$ ./configure
$ make clean check
$ sudo make install
```

libneo4j-client requires some dependencies to build, including OpenSSL and
[libedit](http://thrysoee.dk/editline/). The need for these can be disabled
by invoking configure with `--without-tls` or `--disable-tools` respectively.

Building from the GitHub repository requires a few extra steps. Firstly, some
additional tooling is required, including autoconf, automake, libtool and
[peg/leg](http://piumarta.com/software/peg/). Assuming these are available,
to checkout from GitHub and build:

```console
$ git clone https://github.com/cleishm/libneo4j-client.git
$ cd libneo4j-client
$ ./autogen.sh
$ ./configure
$ make clean check
$ sudo make install
```

NOTE: Recent versions of Mac OS X ship without the OpenSSL header files, and
autoconf doesn't pick this up (yet). If you used the homebrew install method,
this will resolve the issue. If you're using Mac OS X, want to build manually
instead of using homebrew, and you get a build failure related to missing
openssl headers, try the following:

```console
$ brew install openssl
$ ./configure --with-libs=/usr/local/opt/openssl
$ make clean check
$ sudo make install
```

More detail about this workaround can be found via `brew info openssl`.


Support
-------

Having trouble with libneo4j-client? Please raise any issues with usage on
[StackOverflow](http://stackoverflow.com/questions/tagged/libneo4j-client). If
you've found a bug in the code for libneo4j-client, please raise an issue on
[GitHub](https://github.com/cleishm/libneo4j-client) and include details of how
to reproduce the bug.


Contributing
------------

Contributions to libneo4j-client are encouraged and should be made via pull
requests made to the [GitHub repository](
https://github.com/cleishm/libneo4j-client). Please include test cases where
possible, and use a style and approach consistent with the rest of the library.

It may be worthwhile raising an issue on github for the contribution you
intend to make before developing the code, to allow for discussion and feedback
on the requirements.


License
-------

libneo4j-client is licensed under the [Apache License, Version 2.0](
http://www.apache.org/licenses/LICENSE-2.0).

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.  See the License for the
specific language governing permissions and limitations under the License.

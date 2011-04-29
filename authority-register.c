/*
 * Author:: Richard Lehane (mailto:richard.lehane@records.nsw.gov.au)
 * Copyright:: Copyright (c) State of New South Wales through the
 * State Records Authority of New South Wales, 2011
 * License:: GNU General Public License
 * 
 * Simple application that increments Authority and Appraisal Report numbers and versions.
 * Designed for use on a network so that multiple users can acquire new numbers
 * by using the command line options (can be automatically called from Authority Editor).
 * 
 * Requires SQLITE3 and BSTRING libraries.
 * 
 * Compiles on Windows with MingW using the command:
 *   gcc authority-register.c bstring/bstrlib.c sqlite3/sqlite3.c -o authority-register.exe
 *  
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING). If not, see <http://www.gnu.org/licenses/>.
 * 
*/
#include <windows.h>
#include <stdio.h>
#include <getopt.h>
#include "sqlite3/sqlite3.h"
#include "bstring/bstrlib.h"

// Find the executable's directory path
bstring local_dir(char *argv0) {
    bstring exe_path; 
    bstring exe_name = bfromcstr("authority-register.exe");
    bstring empty = bfromcstr("");
    exe_path = bfromcstr(argv0);
  
    bfindreplace(exe_path, exe_name, empty, 0);
   
    bdestroy(exe_name);
    bdestroy(empty);  
    return exe_path;
}

// Execute an SQL statement with a callback function
int execute_c_statement (sqlite3 *db, const char *stmt, 
                                    int (*callback)(void*, int, char**, char**),
                                    void* param) {
    char *DbError = 0;
    int rc;
  
    rc = sqlite3_exec(db, stmt, callback, param, &DbError);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error executing statement: %s\n, Error: %s\n", stmt, DbError);
        sqlite3_free(DbError);
    }
    return 0;
}

// Execute a simple SQL statement
int execute_statement (sqlite3 *db, const char *stmt) {
    return execute_c_statement (db, stmt, NULL, NULL); 
}

// A callback that creates HTML table rows for each row in DB
int htmlify (void* html_string, int argc, char** argv, char** col_name) {
    int i;

    for (i=0; i<argc; i++) {
        if (col_name[i][0] == 'I')
            bcatcstr((bstring)html_string, "<tr>"); 
        bcatcstr((bstring)html_string, "<td>");
        bcatcstr((bstring)html_string, argv[i] ? argv[i]: "Empty");
        bcatcstr((bstring)html_string, "</td>");
        if (col_name[i][0] == 'D')
            bcatcstr((bstring)html_string, "</tr>"); 
    }
    return 0;
}

// Write buffer to file (MS Win)
void windows_write(const char* buffer, int buflen, const char* path) {
    HANDLE winfile; 
    DWORD togo; 
    DWORD written = 0;
    
    winfile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (winfile == INVALID_HANDLE_VALUE) { 
        fprintf(stderr, "Error creating windows file: %d\n", GetLastError());
        return;
    }
    togo = (DWORD)buflen;
    while (written < togo) {
      if (FALSE == WriteFile(winfile, buffer + written, togo - written, &written, NULL)) {
          fprintf(stderr, "Error writing to file: %d\n", GetLastError());
          CloseHandle(winfile);
          return;
        }
    }
  
    CloseHandle(winfile);
    return;
}

// Write all the rows in the database to an HTML file 
// (the default function if no command line options given)
void write_report (bstring exe_path, sqlite3 *db) {
    bstring file_path = bstrcpy(exe_path);
    bstring html = bfromcstr("<html><head>"
        "<title>Authority Register</title>"
        "<style>"
        "body {padding-right: 0px; padding-left: 0px; font-size: 80%;"
        "background: #fff; margin: 6px 12px; color: #000; padding-top: 0px;"
        "font-family: verdana, arial, sans-serif; text-align: left}"
        "h1 {font-weight: 600; margin: 10px 0px 5px; color: #000080;"
        "margin-top: 0px; font-size: 1.6em}"
        "h2 {font-weight: 600; margin: 10px 0px 5px; color: #000080;"
        "margin-top: 10px; font-size: 1.3em}"
        "th {text-align: left}"
        "</style></head>"
        "<body><h1>Authority Register</h1>"
        "<h2>Funtional authorities</h2>"
        "<table width='400'><tr><th>Number</th><th>Version</th>"
        "<th>Date registered</th></tr>");

    execute_c_statement(db, "SELECT * FROM FA ORDER BY ID DESC;", &htmlify, (void*)html);
    bcatcstr(html, "</table><h2>General authorities</h2><table width='400'>"
        "<tr><th>Number</th><th>Version</th><th>Date registered</th></tr>");
    execute_c_statement(db, "SELECT * FROM GA ORDER BY ID DESC;", &htmlify, (void*)html);
    bcatcstr(html, "</table><h2>Appraisal reports</h2><table width='400'>"
        "<tr><th>Number</th><th>Version</th><th>Date registered</th></tr>");
    execute_c_statement(db, "SELECT * FROM AR ORDER BY ID DESC;", &htmlify, (void*)html);
    bcatcstr(html, "</table></body></html>");
    bcatcstr(file_path, "authority-report.html");
    windows_write(bdata(html), blength(html), bdata(file_path));
    
    bdestroy(file_path);
    bdestroy(html);
    return;
}

// Create a new SQLITE database with three tables
void create_db (sqlite3 *db) {
    const char *new_fa =
        "CREATE TABLE IF NOT EXISTS FA ("
        "ID INTEGER PRIMARY KEY,"
        "CurrentVersion INTEGER," 
        "Date TEXT);";
    const char *new_ga =
        "CREATE TABLE IF NOT EXISTS GA ("
        "ID INTEGER PRIMARY KEY,"
        "CurrentVersion INTEGER," 
        "Date TEXT);";
    const char *new_ar =
        "CREATE TABLE IF NOT EXISTS AR ("
        "ID INTEGER PRIMARY KEY,"
        "CurrentVersion INTEGER," 
        "Date TEXT);";
  
    execute_statement(db, new_fa);
    execute_statement(db, new_ga);
    execute_statement(db, new_ar);

    return;
}

// Simple callback designed to print the first result of a SELECT statement
int printresult (void* param, int argc, char** argv, char** col_name) {
    fprintf(stdout, argv[0]);
    return 0;
}

// Insert a new row into table: the ID autoincrements to give new FA/GA/AR number
void register_new(sqlite3 *db, bstring type) {
    bstring stmt;
   
    stmt = bformat("INSERT INTO %s (ID, CurrentVersion, Date) VALUES"
        " (NULL, 1, date('now'));", bdata(type));
    execute_statement(db, bdata(stmt));
   
    bdestroy(stmt);
    return; 
}

// Remove a row from a table: in case an FA/GA/AR is accidentally registered
// If row exists, it is printed to the output.
// Note: the ID 'de-registered' will only be available for re-use if it is the latest 
// row in the table. 
void deregister(sqlite3 *db, bstring type, bstring id) {
    bstring stmt;
   
    stmt = bformat("SELECT ID FROM %s WHERE ID=%s;" 
        "DELETE FROM %s WHERE ID=%s;", bdata(type), bdata(id),
        bdata(type), bdata(id));
    execute_c_statement(db, bdata(stmt), &printresult, NULL);
   
    bdestroy(stmt);
    return; 
}

// Increment the version number for an FA/GA/AR and print result
void increment(sqlite3 *db, bstring type, bstring id) {
    bstring stmt;
   
    stmt = bformat("UPDATE %s SET CurrentVersion = (CurrentVersion + 1)"
        " WHERE ID=%s; SELECT CurrentVersion FROM %s WHERE ID=%s;",
        bdata(type), bdata(id), bdata(type), bdata(id));
    execute_c_statement(db, bdata(stmt), &printresult, NULL);
    
    bdestroy(stmt);
    return; 
}

// Decrement the version number for an FA/GA/AR (if greater than one)
// and print result
void decrement(sqlite3 *db, bstring type, bstring id) {
    bstring stmt;
   
    stmt = bformat("UPDATE %s SET CurrentVersion = (CurrentVersion - 1)"
        " WHERE (ID=%s AND CurrentVersion > 1); SELECT CurrentVersion"
        " FROM %s WHERE ID=%s;", bdata(type), bdata(id), bdata(type), bdata(id));
    execute_c_statement(db, bdata(stmt), &printresult, NULL);
   
    bdestroy(stmt);
    return;  
}

// Seed an FA/GA/AR table with ID's up to the ID given
// Note: works only on empty tables)
void seed(sqlite3 *db, bstring type, bstring id) {
    bstring stmt = bfromcstr("");
    bstring row = bfromcstr("");
    int i;
    int upto = atoi(bdata(id)) + 1;
   
    for (i = 1; i < upto; i++) {
        bassignformat(row, "INSERT INTO %s (ID, CurrentVersion, Date) VALUES"
        " (%d, 1, date('now'));", bdata(type), i);
        bconcat(stmt, row);
    }
    execute_statement(db, bdata(stmt));
    
    bdestroy(stmt);
    bdestroy(row);
    return;  
}

// Take the command line input and if it matches FA/GA or AR return a bstring,
//  otherwise return NULL
bstring make_table_name(const char* value) {
    bstring table_name = bfromcstr(value);
    int achar;
    int bchar;
  
    btrunc(table_name, 2);
    btoupper(table_name);
    if (blength(table_name) == 2) {
        achar = bdata(table_name)[0];
        bchar = bdata(table_name)[1];
        if (((achar == 'F' || achar == 'G' ) && bchar == 'A') || (achar == 'A'
            && bchar == 'R')) {
            return table_name;
        } else {
          bdestroy(table_name);
          return NULL;
        }
    } else {
        bdestroy(table_name); 
        return NULL; 
    }
}

// Take the command line input, trim off the first two characters, if the 
// remaining characters are all digits return a bstring, otherwise return NULL
bstring make_id_number(const char* value) {
    bstring id_number;
    bstring str_value = bfromcstr(value);
    int i;
  
    if (blength(str_value) > 2) {
        id_number = bmidstr(str_value, 2, blength(str_value) - 2);
        bdestroy(str_value);
        for (i = 0; i < blength(id_number); i++) {
            switch (bdata(id_number)[i]) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    break;
                default:
                    bdestroy(id_number);
                    return NULL;
            }
        } 
        return id_number;
    } else {
        bdestroy(str_value);
        return NULL;
    }  
}

int main (int argc, char **argv) {
    sqlite3 *db;
    int rc = 0;
    bstring db_file_path;
    bstring exe_path;
    bstring table_name = NULL;
    bstring id_number = NULL;
    char *usage = "\nExample usage:\n"
        "(Register new AR/FA/GA)              -n AR\n"
        "(Remove AR/FA/GA)            -r FA250\n"
        "(Increment version of an AR/FA/GA) -v GA28\n"
        "(Decrement version of an AR/FA/GA) -d GA28\n"
        "(Seed the AR/FA/GA tables with numbers up to) -s FA249\n";
    int c;
     
    exe_path = local_dir(argv[0]);
    db_file_path = bstrcpy(exe_path);
    bcatcstr(db_file_path, "authority-register.db");
    // if no database exists, try to create it, otherwise open up the existing db
    rc = sqlite3_open_v2(bdata(db_file_path), &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK) {
        rc = sqlite3_open(bdata(db_file_path), &db);
        if (rc == SQLITE_OK) { 
            create_db(db);
        } else {
            fprintf(stderr, "Error: can't create db: %s", bdata(db_file_path));
            bdestroy(db_file_path);
            bdestroy(exe_path);
            return 1;
        }
    }
    // if no arguments given, write a report then exit
    if (argc < 2) {
        write_report(exe_path, db);
        sqlite3_close(db);
        bdestroy(db_file_path);
        bdestroy(exe_path);
        return 0;
    }
  
    while ((c = getopt (argc, argv, "n:r:v:d:s:")) != -1) {
        switch (c) {
            case 'n':
                if (table_name = make_table_name(optarg)) {
                    register_new(db, table_name);
                    fprintf(stdout, "%d", sqlite3_last_insert_rowid(db));
                } else
                    fprintf(stderr, usage);
                break;
            case 'r':
                if ((table_name = make_table_name(optarg)) &&
                    (id_number = make_id_number(optarg))) {
                    deregister(db, table_name, id_number);
                } else
                    fprintf(stderr, usage);
                break;
            case 'v':
                if ((table_name = make_table_name(optarg)) &&
                    (id_number = make_id_number(optarg))) {
                    increment(db, table_name, id_number);
                } else
                    fprintf(stderr, usage);
                break;
            case 'd':
                if ((table_name = make_table_name(optarg)) &&
                    (id_number = make_id_number(optarg))) {
                    decrement(db, table_name, id_number);
                } else
                    fprintf(stderr, usage);
                break;
            case 's':
                if ((table_name = make_table_name(optarg)) &&
                    (id_number = make_id_number(optarg))) {
                    seed(db, table_name, id_number);
                } else
                    fprintf(stderr, usage);
                break;
            default:
                fprintf(stderr, usage);
                break;
        } 
    }
  
    sqlite3_close(db);
    bdestroy(db_file_path);
    bdestroy(exe_path);
    bdestroy(table_name);
    bdestroy(id_number);
    return 0;
}

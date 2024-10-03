#pragma once
inline static const struct SQLRequests
{
    char DropTableDocs[30] = "DROP TABLE IF EXISTS HTMLDocs";
    char DropTableWords[35] = "DROP TABLE IF EXISTS HTMLDocsWords";
    char DropTableIndexer[35] = "DROP TABLE IF EXISTS HTMLIndexer";
    char CreateTableDocs[79] = "CREATE TABLE IF NOT EXISTS HTMLDocs(id SERIAL PRIMARY KEY,docName varchar)";
    char CreateTableWords[84] = "CREATE TABLE IF NOT EXISTS HTMLDocsWords(id BIGSERIAL PRIMARY KEY,word varchar)";
    char CreateTableIndexer[75] = "CREATE TABLE IF NOT EXISTS HTMLIndexer(idDoc int,idWord int,frequency int)";
} requests;

inline static const struct dbStrings
{
    char user[6] = "user=";
    char password[11] = "password=";
    char dbname[8] = "dbname=";
    char host[6] = "host=";
    char dbTblDocsName[9] = "HTMLDocs";
    char dbTblWordsName[14] = "HTMLDocsWords";
    char dbTblIndexerName[12] = "HTMLIndexer";
} dbstr;

inline static const struct INIStrings
{
    char user[14] = "Database.user";
    char password[18] = "Database.password";
    char dbname[16] = "Database.dbname";
    char host[14] = "Database.host";
    char accept[22] = "RequestHeaders.accept";
    char useragent[25] = "RequestHeaders.useragent";
    char searchdepth[19] = "Search.SearchDepth";
    char StartURL[18] = "Search.StarterURL";
    char ServerHost[12] = "Server.host";
    char ServerPort[12] = "Server.port";
} inistr;
//
// Created by logan on 1/11/18.
//

#include <mongocxx/exception/bulk_write_exception.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <iostream>
#include "AbstractMongoDBWriter.h"
#include "../Version.h"

using namespace plugin::ActiveCM_RITAWriter;


AbstractMongoDBWriter::AbstractMongoDBWriter(std::shared_ptr<const mongocxx::client> client) :
        client{std::move(client)} {

}

bool AbstractMongoDBWriter::Init() {
    return true;
}

bool AbstractMongoDBWriter::Rotate() {
    return true;
}

 /** Create Rita's Meta Database Entry for the current batch of logs
  *
  * Adds Metadata to the MongoDB Database that allows RITA to not double 
  * process log files.
  * The MetaDB info includes:
  * - Database name.
  * - Whether the database has been analyzed.
  * - RITA's Use Dates capability (Depricated)
  * - The version of the Analyzer used
  * - The version of the importer used ( this program )
  *
  * The plugin is written in a way to attempt to avoid reinsertion of the meta
  * database, as it will throw an exception, Just in case however the write
  * exception is caught here.
  *
  * \return True if everything went well, false if the Database throws an exception
  *
  */

bool AbstractMongoDBWriter::CreateMetaEntry(const std::string &targetDB) {
    using bsoncxx::builder::basic::kvp;
    mongocxx::collection coll = (*this->client)["MetaDatabase"]["databases"];

    // new MetaDB entry
    bsoncxx::builder::basic::document metaDBDoc;
    metaDBDoc.append(kvp("name", targetDB));
    metaDBDoc.append(kvp("analyzed", false));
    metaDBDoc.append(kvp("dates", false));
    metaDBDoc.append(kvp("analyze_version", std::string("")));
    metaDBDoc.append(kvp("import_version", std::string("v") +
            std::to_string(PLUGIN_MAJOR) + "." +
            std::to_string(PLUGIN_MINOR) + "." +
	    std::to_string(PLUGIN_PATCH) + "+" + PLUGIN_NAME_SEMVER));

    try {
        //assume this is a new database
        coll.insert_one( metaDBDoc.view() );
    } catch (const mongocxx::bulk_write_exception& ){

        //check if the database has already been analyzed
        bsoncxx::builder::basic::document query;
        query.append(kvp("name", targetDB));
        auto existingDoc = coll.find_one(query.view());

        if (!existingDoc || existingDoc.value().view()["analyzed"].get_bool()) {
            //if we couldn't query for the existing record, or the db is already analyzed, error out
            return false;
        }
    }

    return true;
}

  /** Reindexes the database to be searchable more convieniently by RITA
   *
   * Changes the indexing scheme for the MongoDB database from its implicit 
   * indexing to others based on RITA's needs:
   * -CONN
   *    -# Packet timestamp
   *    -# Connection Duration
   *    -# Identifier for referencing packets cross connection type
   * -HTTP
   *    -# User Agent, string representing the:
   *      - Operating system
   *      - Software Vendor/version
   *      - Application Type
   *      of the sender
   *    -# Identifier for referencing packets cross connection type
   * -DNS
   *    -# Name being queried
   *
   *  All types of logs are also sorted based on the originator and 
   *  recipient of the packet.
   *
   */

bool AbstractMongoDBWriter::IndexLogCollection(const std::string &targetDB, const std::string &targetCollection) {
    mongocxx::collection coll = (*this->client)[targetDB][targetCollection];

    std::vector<bsoncxx::document::value> indexes;
    using bsoncxx::builder::basic::kvp;

    if (targetCollection == "conn") {
        bsoncxx::builder::basic::document tsIndex;
        tsIndex.append(kvp("ts", 1));
        indexes.push_back(tsIndex.extract());

        bsoncxx::builder::basic::document durationIndex;
        durationIndex.append(kvp("duration", -1));
        indexes.push_back(durationIndex.extract());

        bsoncxx::builder::basic::document uidIndex;
        uidIndex.append(kvp("uid", 1));
        indexes.push_back(uidIndex.extract());

    } else if (targetCollection == "http") {

        bsoncxx::builder::basic::document userAgentIndex;
        userAgentIndex.append(kvp("user_agent", "hashed"));
        indexes.push_back(userAgentIndex.extract());

        bsoncxx::builder::basic::document uidIndex;
        uidIndex.append(kvp("uid", 1));
        indexes.push_back(uidIndex.extract());

    } else if (targetCollection == "dns") {
        bsoncxx::builder::basic::document queryIndex;
        queryIndex.append(kvp("query", "hashed"));
        indexes.push_back(queryIndex.extract());

    } else {
        return false;
    }

    bsoncxx::builder::basic::document origIndex;
    origIndex.append(kvp("id_orig_h", "hashed"));
    indexes.push_back(origIndex.extract());

    bsoncxx::builder::basic::document respIndex;
    respIndex.append(kvp("id_resp_h", "hashed"));
    indexes.push_back(respIndex.extract());

    for (auto& index: indexes) {
        //TODO: Figure out how to error check this
        coll.create_index(std::move(index));
    }

    return true;
}

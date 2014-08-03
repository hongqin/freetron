/*
 * The main website application
 */

#ifndef H_WEBSITE
#define H_WEBSITE

#include <cppcms/service.h>

#include "date.h"
#include "content.h"
#include "database.h"
#include "../processor.h"

class website : public cppcms::application
{
    Date date;
    Database& db;
    Processor& p;

public:
    // Specify root if this is in a subdirectory, e.g. /website
    website(cppcms::service& srv, Database& db, Processor& p, std::string root = "");

    // Create menu and set template settings
    void init(content::master& c);

    // Site pages
    void home();
    void account();
    void forms();

    // Used when uploading a new form
    void upload(std::string num);
    void process(std::string num);

    // 404 Page
    virtual void main(std::string url);

    // Log the user in, if valid
    bool login(const std::string& user, const std::string& pass);

    // Determine if logged in using session information
    bool loggedIn();
    bool loggedOut();
};

#endif
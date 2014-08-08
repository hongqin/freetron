#include <vector>
#include <cstdlib>
#include <iostream>
#include <cppcms/application.h>
#include <cppcms/mount_point.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/applications_pool.h>
#include <booster/system_error.h>
#include <booster/intrusive_ptr.h>
#include <boost/bind.hpp>

#include "rpc.h"
#include "options.h"
#include "content.h"

rpc::rpc(cppcms::service& srv, Database& db, Processor& p)
    : cppcms::rpc::json_rpc_server(srv), db(db), p(p),
      timer(srv.get_io_service()), exiting(false),
      statusThread(StatusThread(*this, exiting))
{
    // Random, used for form confirmations
    srand(time(NULL));

    // Account
    bind("account_login", cppcms::rpc::json_method(&rpc::account_login, this), method_role);
    bind("account_logout", cppcms::rpc::json_method(&rpc::account_logout, this), method_role);
    bind("account_create", cppcms::rpc::json_method(&rpc::account_create, this), method_role);
    bind("account_update", cppcms::rpc::json_method(&rpc::account_update, this), method_role);
    bind("account_delete", cppcms::rpc::json_method(&rpc::account_delete, this), method_role);

    // Forms
    bind("form_process", cppcms::rpc::json_method(&rpc::form_process, this), method_role);
    bind("form_getone", cppcms::rpc::json_method(&rpc::form_getone, this), method_role);
    bind("form_getall", cppcms::rpc::json_method(&rpc::form_getall, this), method_role);
    bind("form_delete", cppcms::rpc::json_method(&rpc::form_delete, this), method_role);
    bind("form_rename", cppcms::rpc::json_method(&rpc::form_rename, this), method_role);

    // Timeouts for getting rid of long requests
    on_timer(booster::system::error_code());
}

rpc::~rpc()
{
    timer.reset_io_service();

    // We need to tell Processor to wake up all the statusWait() calls we're
    // waiting on for status updates so we can exit that thread
    exiting = true;
    p.statusWakeAll();
    statusThread.join();
}

void rpc::account_login(const std::string& user, const std::string& pass)
{
    if (loggedIn())
        logout();

    if (!user.empty() && !pass.empty() && session().is_set("prelogin"))
    {
        User u(user, pass);

        if (u.valid() && login(user, pass))
        {
            return_result(true);
            return;
        }
    }

    return_result(false);
}

void rpc::account_logout()
{
    logout();
    return_result(true);
}

void rpc::account_create(const std::string& user, const std::string& pass)
{
    if (loggedOut() && !user.empty() && !pass.empty())
    {
        User u(user, pass);

        if (u.valid() && db.addUser(user, pass) > 0 && login(user, pass))
        {
            return_result(true);
            return;
        }
    }

    return_result(false);
}

void rpc::account_update(const std::string& user, const std::string& pass)
{
    if (loggedIn() && !user.empty() && !pass.empty())
    {
        User u(user, pass);
        long long id = session().get<long long>("id");

        if (u.valid() && db.updateAccount(user, pass, id))
        {
            session().set<std::string>("user", user);

            return_result(true);
            return;
        }
    }

    return_result(false);
}

void rpc::account_delete(int confirmation)
{
    if (loggedIn() && confirmation == session().get<int>("confirmation"))
    {
        long long id = session().get<long long>("id");

        if (db.deleteAccount(id))
        {
            logout();

            return_result(true);
            return;
        }
    }

    return_result(false);
}

void rpc::form_process(long long formId)
{
    cppcms::json::value v = cppcms::json::object();
    cppcms::json::object& obj = v.object();

    // Send the ID again so we can easily create the next request
    obj["id"] = formId;
    obj["percent"] = 0;

    if (loggedIn())
    {
        if (p.done(formId))
        {
            obj["percent"] = 100;
        }
        else
        {
            // If it's not done, then save this request and process it once we
            // are notified that this form is done, or if a timeout occurs
            std::unique_lock<std::mutex> lock(waiters_mutex);
            booster::shared_ptr<cppcms::rpc::json_call> call = release_call();
            waiters.push_back(ProcessRequest(formId, call));

            // If the connection is closed before it's done, remove the request
            call->context().async_on_peer_reset(
                    boost::bind(
                        &rpc::remove_context,
                        booster::intrusive_ptr<rpc>(this),
                        call));
            return;
        }
    }

    return_result(v);
}

void rpc::form_getone(long long formId)
{
    cppcms::json::value v = cppcms::json::array();

    if (loggedIn())
        getForms(v, formId);

    return_result(v);
}

void rpc::form_getall()
{
    cppcms::json::value v = cppcms::json::array();

    if (loggedIn())
        getForms(v);

    return_result(v);
}

void rpc::getForms(cppcms::json::value& v, long long formId)
{
    long long userId = session().get<long long>("id");

    // If formId == 0, then it'll get all the forms
    std::vector<FormData> forms = db.getForms(userId, formId);

    // Optimization
    cppcms::json::array& ar = v.array();
    ar.reserve(forms.size());

    for (const FormData& f : forms)
    {
        cppcms::json::object obj;
        obj["id"] = f.id;
        obj["key"] = f.key;
        obj["name"] = f.name;
        obj["data"] = (f.data.empty())?"Processing... please wait":f.data;
        obj["date"] = f.date;
        ar.push_back(obj);
    }
}

void rpc::form_delete(long long formId)
{
    if (loggedIn())
    {
        long long userId = session().get<long long>("id");

        if (db.deleteForm(userId, formId))
        {
            return_result(true);
            return;
        }
    }

    return_result(false);
}

void rpc::form_rename()
{
    if (loggedIn())
    {
    }

    return_result(false);
}

bool rpc::loggedIn()
{
    if (session().is_set("loggedIn") && session().get<bool>("loggedIn") &&
        db.idExists(session().get<long long>("id")))
        return true;

    return false;
}

bool rpc::loggedOut()
{
    return !loggedIn();
}

bool rpc::login(const std::string& user, const std::string& pass)
{
    // See if valid user
    long long id = db.validUser(user, pass);

    if (id > 0)
    {
        session().reset_session();
        session().erase("prelogin");
        session().set<bool>("loggedIn", true);
        session().set<long long>("id", id);
        session().set<std::string>("user", user);
        resetCode();

        return true;
    }

    return false;
}

void rpc::logout()
{
    if (loggedIn())
    {
        session().set<bool>("loggedIn", false);

        session().erase("id");
        session().erase("user");
        session().erase("loggedIn");
    }

    session().reset_session();
}

void rpc::resetCode()
{
    session().set<int>("confirmation", rand());
}

void rpc::on_timer(const booster::system::error_code& e)
{
    // Cancel
    if (e)
        return;

    // In seconds
    int timeout = 15*60; // 15 min

    // Remove really old connections
    std::unique_lock<std::mutex> lock(waiters_mutex);

    for (std::vector<ProcessRequest>::iterator i = waiters.begin();
            i != waiters.end(); ++i)
    {
        if (time(NULL) - i->createTime > timeout)
        {
            i->call->return_error("Connection closed by server");
            i = waiters.erase(i);

            if (i == waiters.end())
                break;
        }
    }

    // Restart timer
    timer.expires_from_now(booster::ptime::seconds(60));
    timer.async_wait(boost::bind(&rpc::on_timer, booster::intrusive_ptr<rpc>(this), _1));
}

void rpc::remove_context(booster::shared_ptr<cppcms::rpc::json_call> call)
{
    std::unique_lock<std::mutex> lock(waiters_mutex);
    std::vector<ProcessRequest>::iterator i = std::find_if(
            waiters.begin(), waiters.end(), RequestCallPredicate(call));

    if (i != waiters.end())
        waiters.erase(i);
}

void rpc::broadcast(long long formId, int percentage)
{
    cppcms::json::value v = cppcms::json::object();
    cppcms::json::object& obj = v.object();

    // Send the ID again so we can easily create the next request
    obj["id"] = formId;
    obj["percent"] = percentage;

    std::unique_lock<std::mutex> lock(waiters_mutex);

    for (std::vector<ProcessRequest>::iterator i = waiters.begin();
            i != waiters.end(); ++i)
    {
        if (i->formId == formId)
        {
            i->call->return_result(v);

            // Remove this under the assumption that there is only one request
            // per formId
            waiters.erase(i);
            break;
        }
    }
}

void StatusThread::operator()()
{
    while (!exiting)
    {
        std::vector<Status> results = parent.p.statusWait();

        if (exiting)
            break;

        for (Status& s : results)
            parent.broadcast(s.formId, s.percentage);
    }
}

#ifndef DAQI_CONTEXT_HPP
#define DAQI_CONTEXT_HPP

#include <memory>
#include <functional>
#include <list>

#include "daqi/def/def.hpp"
#include "daqi/def/asio_def.hpp"
#include "daqi/def/json_def.hpp"

#include "daqi/request.hpp"
#include "daqi/response.hpp"
#include "daqi/templates.hpp"
#include "daqi/intercepter.hpp"
#include "daqi/rediscli_pool.hpp"

namespace da4qi4
{

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

class ContextIMP;
using Context = std::shared_ptr<ContextIMP>;

class Application;

class ContextIMP
    : public std::enable_shared_from_this<ContextIMP>
{
    ContextIMP(ConnectionPtr cnt);
public:
    static Context Make(ConnectionPtr cnt);
    ~ContextIMP();

    ContextIMP(ContextIMP const&) = delete;
    ContextIMP& operator = (ContextIMP const&) = delete;

    Request const& Req() const;
    Response& Res();

    std::string const& Req(std::string const& name) const
    {
        return Req().GetParameter(name);
    }

    Application& App();
    Application const& App() const;

    Json const& Data(std::string const& name) const
    {
        return const_cast<ContextIMP*>(this)->Data(name);
    }

    Json& Data(std::string const& name)
    {
        auto it = _data.find(name);

        if (it == _data.end())
        {
            return theEmptyJson;
        }

        return *it;
    }

    Json LoadData(std::string const& name) const
    {
        return Data(name);
    }

    void SaveData(std::string const& name, Json const& data)
    {
        assert(!name.empty());
        _data[name] = data;
    }

    void RemoveData(std::string const& name)
    {
        auto it = _data.find(name);

        if (it != _data.end())
        {
            _data.erase(it);
        }
    }

    Json& PageData()
    {
        return Data(page_data_name);
    }

    Json const& PageData() const
    {
        return Data(page_data_name);
    }

    Json LoadPageData() const
    {
        return PageData();
    }

    void SavePageData(Json const& data)
    {
        PageData() = data;
    }

    static std::string const& PageDataName()
    {
        return page_data_name;
    }

    Json& SessionData()
    {
        return Data(session_data_name);
    }

    Json const& SessionData() const
    {
        return Data(session_data_name);
    }

    Json LoadSessionData() const
    {
        return SessionData();
    }

    void SaveSessionData(Json const& data)
    {
        SessionData() = data;
    }

    static std::string const& SessionDataName()
    {
        return session_data_name;
    }

    IOC& IOContext();
    size_t IOContextIndex() const;

    log::LoggerPtr Logger()
    {
        return logger();
    }
public:
    void InitRequestPathParameters(std::vector<std::string> const& names
                                   , std::vector<std::string> const& values);

public:
    void Render();

    void RenderWithData(http_status status, Json const& data);
    void RenderWithData(std::string const& template_name, Json const& data);
    void RenderWithData(Json const& data);

public:
    void RenderWithoutData(http_status status)
    {
        RenderWithData(status, theEmptyJson);
    }
    void RenderWithoutData(std::string const& template_name)
    {
        RenderWithData(template_name, theEmptyJson);
    }
    void RenderWithoutData()
    {
        RenderWithData(theEmptyJson);
    }

public:
    void RenderNofound(Json const& data = theEmptyJson)
    {
        RenderWithData(HTTP_STATUS_NOT_FOUND, data);
    }

    void RenderBadRequest(Json const& data = theEmptyJson)
    {
        RenderWithData(HTTP_STATUS_BAD_REQUEST, data);
    }

    void RenderUnauthorized(Json const& data = theEmptyJson)
    {
        RenderWithData(HTTP_STATUS_UNAUTHORIZED, data);
    }

    void RenderForbidden(Json const& data = theEmptyJson)
    {
        RenderWithData(HTTP_STATUS_FORBIDDEN, data);
    }

    void RenderNotImplemented(Json const& data = theEmptyJson)
    {
        RenderWithData(HTTP_STATUS_NOT_IMPLEMENTED, data);
    }

    void RenderServiceUnavailable(Json const& data = theEmptyJson)
    {
        RenderWithData(HTTP_STATUS_SERVICE_UNAVAILABLE, data);
    }

    void RenderInternalServerError(Json const& data = theEmptyJson)
    {
        RenderWithData(HTTP_STATUS_INTERNAL_SERVER_ERROR, data);
    }

public:
    bool HasRedis() const
    {
        return _redis != nullptr;
    }

    RedisClientPtr Redis()
    {
        return _redis;
    }

public:
    void Start();
    void Pass();
    void Stop();

public:
    void StartChunkedResponse();
    void NextChunkedResponse(std::string const& body);
    void StopChunkedResponse();

private:
    void do_intercepter_on_req_dir();
    void do_intercepter_on_res_dir();

    void next(Intercepter::Result result);
    void next_intercepter_on_req_dir(Intercepter::Result result);
    void start_intercepter_on_res_dir(Intercepter::Result result);
    void next_intercepter_on_res_dir(Intercepter::Result result);

private:
    void end();

private:
    log::LoggerPtr logger();

private:
    using Self = ContextIMP;
    typedef std::string const& (Self::*PSSFun)(std::string const&) const;
    void RegistStringFunctionWithOneStringParameter(char const* function_name,
                                                    PSSFun func,
                                                    std::string defaultValue = Utilities::theEmptyString
                                                   );

    typedef bool (Self::*PBSFun)(std::string const&) const;
    void RegistBoolFunctionWithOneStringParameter(char const* function_name,
                                                  PBSFun func, bool defaultValue = false);

private:
    void render_on_template(std::string const& templ_name, Template const& templ, Json const& data, http_status status);
    std::string render_on_template(std::string const& templ_name, Template const& templ, Json const& data
                                   , bool& server_render_error
                                   , std::string& error_detail);

    void regist_template_enginer_common_functions();

    std::string const& parameter(std::string const& name) const
    {
        return this->Req().GetParameter(name);
    }

    bool is_exists_parameter(std::string const& name) const
    {
        return this->Req().IsExistsParameter(name);
    }

    std::string const& header(std::string const& field) const
    {
        return this->Req().GetHeader(field);
    }

    bool is_exists_header(std::string const& field) const
    {
        return this->Req().IsExistsHeader(field);
    }

    std::string const& url_parameter(std::string const& name) const
    {
        return this->Req().GetUrlParameter(name);
    }

    bool is_exists_url_parameter(std::string const& name) const
    {
        return this->Req().IsExistsUrlParameter(name);
    }

    std::string const& path_parameter(std::string const& name) const
    {
        return this->Req().GetPathParameter(name);
    }

    bool is_exists_path_parameter(std::string const& name) const
    {
        return this->Req().IsExistsPathParameter(name);
    }

    std::string const& form_data(std::string const& name) const
    {
        return this->Req().GetFormData(name);
    }

    bool is_exists_form_data(std::string const& name) const
    {
        return this->Req().IsExistsFormData(name);
    }

    std::string const& cookie(std::string const& name) const
    {
        return this->Req().GetCookie(name);
    }

    bool is_exists_cookie(std::string const& name) const
    {
        return this->Req().IsExistsCookie(name);
    }
private:
    ConnectionPtr _cnt;

    Json _data;

    inja::Environment _env;

    Intercepter::On _intercepter_on;
    Intercepter::ChainIterator  _intercepter_iter;
    Intercepter::ChainIterator _intercepter_beg, _intercepter_end;

    RedisClientPtr _redis;

private:
    static std::string session_data_name;
    static std::string page_data_name;
};


} //namespace da4qi4


#endif // DAQI_CONTEXT_HPP
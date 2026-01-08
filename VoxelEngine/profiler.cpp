#include "profiler.h"

Profiler::Profiler()
    : m_CurrentSession(nullptr)
    , m_Enabled(true)
{}

Profiler::~Profiler()
{
    EndSession();
}

Profiler& Profiler::GetInstance()
{
    static Profiler instance;
    return instance;
}

void Profiler::BeginSession(const std::string& name, const std::string& filepath)
{
    LOG_INFO(EngineSystem::CORE, "Starting profiler session '{}'", name);

    std::lock_guard lock(m_Mutex);
    if (m_CurrentSession)
    {
        LOG_WARN(EngineSystem::CORE,
            "Profiler '{}' begin session called when '{}' session was already open",
            name, m_CurrentSession->Name);
        _InternalEndSession();
    }

    m_OutputStream.open(filepath);

    if (m_OutputStream.is_open())
    {
        m_CurrentSession = new ProfilerSession({ name });
        _WriteHeader();
    }
    else
    {
        LOG_ERROR(EngineSystem::CORE,
            "Profiler could not open result file '{}'", filepath);
    }
}

void Profiler::EndSession()
{
    std::lock_guard lock(m_Mutex);

    if (!m_CurrentSession)
    {
        LOG_WARN(EngineSystem::CORE, "Profiler end session called when no session was open");
    }
    else
    {
        LOG_INFO(EngineSystem::CORE, "Ending profiler session '{}'", m_CurrentSession->Name);
    }

    _InternalEndSession();
}

void Profiler::WriteProfile(const ProfileResult& result)
{
    std::stringstream json;

    json << std::setprecision(3) << std::fixed;
    json << ",{";
    json << "\"cat\":\"function\",";
    json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
    json << "\"name\":\"" << result.Name << "\",";
    json << "\"ph\":\"X\",";
    json << "\"pid\":0,";
    json << "\"tid\":" << result.ThreadID << ",";
    json << "\"ts\":" << result.Start.count();
    json << "}";

    std::lock_guard lock(m_Mutex);
    if (m_CurrentSession)
    {
        m_OutputStream << json.str();
        m_OutputStream.flush();
    }
}

void Profiler::SetEnabled(bool isEnabled) 
{ 
    m_Enabled = isEnabled; 

    LOG_INFO(EngineSystem::CORE,
        "Profiler Status: {}",
        isEnabled ? "Enabled" : "Disabled");
}

void Profiler::_WriteHeader()
{
    m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
    m_OutputStream.flush();
}

void Profiler::_WriteFooter()
{
    m_OutputStream << "]}";
    m_OutputStream.flush();
}

void Profiler::_InternalEndSession()
{
    if (m_CurrentSession)
    {
        _WriteFooter();
        m_OutputStream.close();
        delete m_CurrentSession;
        m_CurrentSession = nullptr;
    }
}

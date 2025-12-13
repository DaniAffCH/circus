#include "Container.h"

#include <cassert>

#include "Constants.h"

#define POST "POST"
#define GET "GET"
#define DELETE "DELETE"
#define PUT "PUT"


/*
Endpoints and parameters:
https://docs.docker.com/reference/api/engine/version/v1.39/
*/

#define CREATE_OK_RESPONSE 201
#define START_OK_RESPONSE 204
#define STOP_OK_RESPONSE 204
#define DELETE_OK_RESPONSE 204

namespace spqr {



inline std::string create_container_endpoint(const std::string& name) {
    return "/containers/create?name=" + name;
}

inline std::string start_container_endpoint(const std::string& id) {
    return "/containers/" + id + "/start";
}

inline std::string stop_container_endpoint(const std::string& id) {
    return "/containers/" + id
           + "/stop?t=0";  // TODO: forcing a SIGKILL trigger to 0 seconds as workaround. If the application
                           // is well behaved, this shouldn't be necessary
}

inline std::string remove_container_endpoint(const std::string& id) {
    return "/containers/" + id;
}

Container::Container(const std::string& name, const std::string& sockPath)
    : name(name), sockPath(sockPath), state(ContainerState::NONE) {
    curl_handle = curl_easy_init();
    if (!curl_handle)
        throw std::runtime_error("Failed to init curl handle");
}

Container::~Container() {
    switch (state) {
        case ContainerState::RUNNING:
            stop();
            remove();
            break;
        case ContainerState::IDLE:
            remove();
            break;
        case ContainerState::REMOVED:
            break;
        case ContainerState::NONE:
            break;
    }

    if (curl_handle)
        curl_easy_cleanup(curl_handle);
}

bool checkDockerImage(const std::string& imageName) {
    std::string command = "docker image inspect " + imageName + " > /dev/null 2>&1";
    int status = system(command.c_str());

    if (status != 0) {
        std::string pullCommand = "docker pull " + imageName + " > /dev/null 2>&1";
        return system(pullCommand.c_str()) == 0;
    }

    return true;
}

void Container::create(const std::string& robot_name, const std::string& image,
                       const std::vector<std::string>& binds) {
    nlohmann::json payload;
    payload["Image"] = image;

    bool verifyDockerImage = checkDockerImage(image);
    if (!verifyDockerImage) {
        throw std::runtime_error("Docker image not found and error during pull the: " + image);
    }
    // Forcing networkMode to host is necessary to establish a communication between simulator and docker
    // container.
    payload["HostConfig"] = {{"NetworkMode", "host"}, {"Binds", binds}};

    payload["Env"]
        = {"ROBOT_NAME=" + robot_name, "CIRCUS_PORT=" + std::to_string(frameworkCommunicationPort)};

    std::string endpoint = create_container_endpoint(name);
    std::string resp_raw = request(POST, endpoint, CREATE_OK_RESPONSE, &payload);

    nlohmann::json resp = nlohmann::json::parse(resp_raw);
    if (!resp.contains("Id"))
        throw std::runtime_error("Docker create failed");

    id = resp["Id"];
    state = ContainerState::IDLE;
}

void Container::start() {
    if (state != ContainerState::IDLE)
        throw std::runtime_error("Failed to start container " + name + " which is not IDLE.");

    const std::string endpoint = start_container_endpoint(id);
    request(POST, endpoint, START_OK_RESPONSE);
    state = ContainerState::RUNNING;
}

void Container::stop() {
    if (state != ContainerState::RUNNING)
        throw std::runtime_error("Failed to stop container " + name + " which is not RUNNING.");

    const std::string endpoint = stop_container_endpoint(id);
    request(POST, endpoint, 0);
    state = ContainerState::IDLE;
}

void Container::remove() {
    if (state != ContainerState::IDLE)
        throw std::runtime_error("Failed to remove container " + name + " which is not IDLE.");

    const std::string endpoint = remove_container_endpoint(id);
    request(DELETE, endpoint, DELETE_OK_RESPONSE);
    state = ContainerState::REMOVED;
}


std::string Container::request(const std::string& method, const std::string& endpoint,
                               const long expected_response, const nlohmann::json* body) {
    curl_easy_reset(curl_handle);
    std::string url = "http://localhost" + endpoint;

    curl_easy_setopt(curl_handle, CURLOPT_UNIX_SOCKET_PATH, sockPath.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, method.c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

    const auto write_callback = +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* resp = static_cast<std::string*>(userdata);
        resp->append(ptr, size * nmemb);
        return size * nmemb;
    };

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    std::string response;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);

    std::string postData;
    if (body != nullptr) {
        postData = body->dump();
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, postData.size());
    }

    long response_code = 0;
    CURLcode res = curl_easy_perform(curl_handle);
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    curl_slist_free_all(headers);

    if (res != CURLE_OK)
        throw std::runtime_error(std::string("Curl error: ") + curl_easy_strerror(res));

    if (expected_response && response_code != expected_response)
        throw std::runtime_error("Docker API request to " + endpoint + " failed: HTTP "
                                 + std::to_string(response_code) + ". " + response);

    return response;
}
}  // namespace spqr

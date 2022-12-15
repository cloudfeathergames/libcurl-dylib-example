#include <string>
#include <map>
#include <cstdlib>

class RestClient {
public:

	// members

	typedef std::map<std::string, std::string> headermap;

	typedef struct {
		int code;
		std::string body;
		headermap headers;
	} response;

	typedef struct {
		const char* data;
		size_t length;
	} uploadObject;

	// methods

	void init();

	static response get(const std::string& url, const size_t timeout = 0);
	static response get(const std::string& url, const headermap& headers, const size_t timeout = 0);

	static response post(const std::string& url, const std::string& ctype, const std::string& data, const size_t timeout = 0);
	static response post(const std::string& url, const std::string& ctype, const std::string& data, headermap& headers, const size_t timeout = 0);


private:

	// members

	static const char* userAgent;

	// methods

	static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata);
	static size_t header_callback(void* ptr, size_t size, size_t nmemb, void* userdata);
	static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* userdata);
	// trim from start
	static inline std::string& ltrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {return !std::isspace(c); }));
		return s;
	}

	// trim from end
	static inline std::string& rtrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) {return !std::isspace(c); }).base(), s.end());
		return s;
	}

	// trim from both ends
	static inline std::string& trim(std::string& s) {
		return ltrim(rtrim(s));
	}
};
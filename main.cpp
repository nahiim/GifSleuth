
#include <iostream>
#include <string>
#include <sstream>
#include <asio.hpp>
#include <nlohmann/json.hpp>


const std::string ENDPOINT     = "/v1/stickers/search";
const std::string API_KEY      = "H9Tiu5xjz9nCpPvESXQ5XP07SXlzEyiy";
const std::string SDK_KEY      = "eyeNPLntyMGOc7UldUFMM7fTVKFgz8YW";
const std::string HOST         = "api.giphy.com";
const std::string HTTP_VERSION = "HTTP/1.1";



std::vector<std::string> convertStringToVector(const std::string& str)
{
    std::vector<std::string> words;
    std::string word;
    std::istringstream iss(str);

    // Extract words from the string
    while (iss >> std::ws >> word)
    {
        words.push_back(word);
    }
    
    return words;
}



std::vector<std::string> pullStickers(std::string request)
{
    std::vector<std::string> urls;

    // Define context for connecting
    asio::io_context io_context;

    asio::ip::tcp::resolver resolver(io_context);
    asio::ip::tcp::resolver::results_type endpoints;

    try
    {
        endpoints = resolver.resolve(HOST, "http");
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nEndpoint resolution failed: \t" << e.what() << std::endl;
    }


    asio::ip::tcp::socket socket(io_context);


    try
    {
        asio::connect(socket, endpoints);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Endpoint resolution failed: \t" << e.what() << std::endl;
    }



    try
    {
        // send request
        asio::write(socket, asio::buffer(request));
    }
    catch (const std::exception& e)
    {
        std::cerr << "Endpoint resolution failed: \t" << e.what() << std::endl;
    }

    // receive response
    asio::streambuf response_buffer;
    try
    {
        asio::read(socket, response_buffer, asio::transfer_all());
    }
    catch (const asio::system_error& e)
    {
        if (e.code() == asio::error::eof)
        {
            std::cerr << "\nConnection closed by the server.\n" << std::endl;
        }
        else
        {
            std::cerr << "Error occurred during read: " << e.what() << std::endl;
        }
    }

    // Convert response data to char*
    std::string response_string = asio::buffer_cast<const char*>(response_buffer.data());


    // Get the response status code
    unsigned int status_code = 0;
    if (response_string.substr(0, 9) == "HTTP/1.1 ")
    {
        status_code = std::stoi(response_string.substr(9, 3));
    }

    // Print the response status code
    std::cout << "\nResponse Status Code: " << status_code << "\n" << std::endl;


    // Find the position of the JSON data within the response string (HTTP response always ends with \r\n\r\n)
    auto pos = response_string.find("\r\n\r\n");
    if (pos != std::string::npos)
    {
        // Extract the JSON data from the response string
        std::string json_data = response_string.substr(pos + 4);
        // std::cout << "JSON Data: " << json_data << std::endl;  // Print the JSON data for debugging purposes

        // Parse the JSON data 
        try
        {
            nlohmann::json json_object = nlohmann::json::parse(json_data);

            // Extract urls from JSON data
            for (const auto& gif : json_object["data"])
            {
                urls.push_back(gif["images"]["original"]["url"]);
            }
        }
        catch (const nlohmann::json::parse_error& e)
        {
            std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        }
    }


    return urls;
}



std::vector<std::vector<std::string>> splitUrlsIntoPages(const std::vector<std::string>& urls, int page_size)
{
    std::vector<std::vector<std::string>> pages;

    // Evaluate the total number of pages
    int total_pages = urls.size() / page_size;
    if (urls.size() % page_size != 0)
    {
        total_pages++;
    }

    // Split URLs into pages
    for (int i = 0; i < total_pages; i++)
    {
        std::vector<std::string> page;
        for (int j = i * page_size; j < (i + 1) * page_size && j < urls.size(); j++)
        {
            page.push_back(urls[j]);
        }
        pages.push_back(page);
    }


    return pages;
}


void nextPage(const std::vector<std::vector<std::string>>& search_result, unsigned int& current_page_index, const std::string& query)
{
    if (current_page_index >= search_result.size())
    {
        std::cout << "\n\tYou have viewed the last page" << std::endl;
    }
    else
    {
        std::cout << "\n\tSearch Reasults for \"" << query << "\"\n\tPage " << current_page_index << ":\n" << std::endl;
        // Display the data page at the current index
        const std::vector<std::string>& current_page = search_result[current_page_index];
        for (const std::string& result : current_page)
        {
            std::cout << result << "\n" << std::endl;
        }
        // Increment the current page index
        current_page_index++;
    }
}











int main()
{
            std::cout << R"(    ====================
        GIF SLEUTH
    ====================

    Find and share a world of animated GIFs.)" << std::endl;
        std::cout << "\n\t";



    std::string query;
    // std::string limit  = "100";
    // std::string offset = "0";
    // std::string rating = "g";
    // std::string lang   = "en";
    // std::string bundle = "messaging_non_clips";

    // Number of search results per page
    int page_size = 5;


    std::string command;
    std::vector<std::string> search_results;
    bool search_in_progress = false;
    unsigned int current_page;
    std::vector<std::vector<std::string>> pages;

    while (true)
    {
            std::cout << R"(
    Command List:

    SEARCH <keyword>  - Search gifs using specified keyword.
    NEXT              - Present the next search page.
    RANK              - Display how many stickers with the same rank in search results.
    CANCEL            - Cancel ongoing search.
    EXIT or QUIT      - Close app.)" << std::endl;
        std::cout << "\n\t>>>";
        std::getline(std::cin, command);

        std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (command.substr(0, 7) == "search ")
        {
            try
            {
                query = convertStringToVector(command)[1];
            }
            catch (const std::exception& e)
            {
                std::cerr << "\n\tINVALID COMMAND! \t" << e.what() << std::endl;
            }

            search_in_progress = false;
            current_page = 1;

            std::string request;

            // Cancel ongoing search if any
            if (search_in_progress)
            {
                std::cout << "\n\tCanceling ongoing search..." << std::endl;
                search_results.clear();
                search_in_progress = false;
            }

            request = "GET " + ENDPOINT;
            request += "?api_key=" + API_KEY;
            request += "&q="       + query;
            // request += "&limit="   + limit;
            // request += "&offset="  + offset;
            // request += "&rating="  + rating;
            // request += "&lang="    + lang;
            // request += "&bundle="  + bundle;
            request += " " + HTTP_VERSION + "\r\n";
            request += "Host: " + HOST + "\r\n";
            request += "Connection: close\r\n";
            request += "\r\n";

            // Pull stickers from API Endpoint
            // Store the search results in a vector
            search_results = pullStickers(request);

            pages = splitUrlsIntoPages(search_results, page_size);
            nextPage(pages, current_page, query);

            search_in_progress = true;
        }
        else if (command == "next")
        {
            if (search_in_progress)
            {
                // Display next pay if there is
                if (search_results.empty())
                {
                    std::cout << "\n\tYou have reached the end of the search results." << std::endl;
                }
                else
                {
                    nextPage(pages, current_page, query);
                }
            }
            else
            {
                std::cout << "\n\tNo ongoing search. Please perform a search first." << std::endl;
            }
        }
        else if (command == "cancel")
        {
            if (search_in_progress)
            {
                std::cout << "Canceling search..." << std::endl;
                search_results.clear();
                search_in_progress = false;
            }
            else
            {
                std::cout << "\n\tNo ongoing search to cancel." << std::endl;
            }
        }
        else if (command == "rank")
        {
            if(search_in_progress)
            {
                // Map to store rank counts
                std::map<int, int> rank_count;

                for(const std::vector<std::string>& page : pages)
                {
                    for(const std::string& url : page)
                    {
                        // Extract the rank from the URL
                        // and increment the count for that rank
                        int rank = url[13];
                        rank_count[rank]++;
                    }
                }
                // Print the rank counts
                for (const auto& [rank, count] : rank_count)
                {
                    std::cout << "\tRank " << static_cast<char>(rank) << ": " << count << " URLs" << std::endl;
                }
            }
            else
            {
                std::cout << "\n\tNo ongoing search. Please perform a search first." << std::endl;
            }
        }
        else if (command == "exit" || command == "quit")
        {
            // Exit the program
            std::exit(0);
        }
        else
        {
            std::cout << "\n\tINVALID COMMAND!" << std::endl;
        }
    }


    return 0;
}
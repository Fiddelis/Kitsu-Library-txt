#include<iostream>
#include<string>
#include<fstream>
#include<curl/curl.h>
#include<jsoncpp/json/json.h>

// Abre o arquivo de texto para escrita
std::ofstream txt("animes_library.txt");

// Função de callback para escrever dados recebidos pela requisição CURL em um buffer
size_t writeCallback(char* data, size_t size, size_t nmemb, std::string* buffer) {
    size_t totalSize = size * nmemb;
    buffer->append(data, totalSize);
    return totalSize;
}

// Função para fazer uma requisição CURL para o bter os dados da API
std::string getCurl(std::string link) {

    CURLcode res;
    CURL *curl = curl_easy_init();
    
    std::string responseData;

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(curl, CURLOPT_URL, link.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
            std::cout << "Erro na chamada da API" << std::endl;
        }
    
    curl_easy_cleanup(curl);

    return responseData;
}

// Função para extrair informações relevantes do JSON recebido da API e escrevê-las no arquivo de texto
void infoAnime(std::string responseData, int i) {
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();

    Json::Value root;
    std::string parseErrors;
    bool parsingSuccessful = reader->parse(responseData.c_str(), responseData.c_str() + responseData.size(), &root, &parseErrors);

    delete reader;

    if (!parsingSuccessful) {
        std::cout << "Failed to parse JSON: " << parseErrors << std::endl;
    }

    const Json::Value& dataArray = root["data"];

    const std::string canonicalTitle = dataArray["attributes"]["canonicalTitle"].asString();
    const std::string linkAnime = "https://kitsu.io/anime/" + dataArray["attributes"]["slug"].asString();

    // Escreve os dados relevantes no arquivo de texto
    if(dataArray){
        txt << '[' << i << ']';
        txt << " Título Canônico: " << canonicalTitle << "\n";
        txt << "Link: " << linkAnime << "\n\n";

        // Progresso
        std::cout << '.' << std::flush;
    }
}

// Função para ler os dados JSON recebidos da API e extrair as informações de cada anime
void readerJson(std::string responseData, int i) {

    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    Json::Value root;

    std::string parseErrors;

    bool parsingSuccessful = reader->parse(responseData.c_str(), responseData.c_str() + responseData.size(), &root, &parseErrors);

    delete reader;

    if (!parsingSuccessful) {
        std::cout << "Failed to parse JSON: " << parseErrors << std::endl;
    }

    const Json::Value& dataArray = root["data"];

    // Itera através dos dados JSON recebidos para cada anime
    for (const Json::Value& entry : dataArray) {
        const Json::Value& anime = entry["relationships"]["anime"];
        std::string animeLink = anime["links"]["related"].asString();

        // Obtém informações detalhadas do anime
        infoAnime(getCurl(animeLink), i);
        
        // Incrementa o contador de anime
        i++;
    }

    // Verifica se há mais páginas de resultados
    if (root["links"]["next"].isNull()) {
            std::cout << "Completo" << std::endl;
            return; // Não há mais páginas
        }

    std::string nextPage = getCurl(root["links"]["next"].asString());
    std::cout << i << std::endl;
    readerJson(nextPage, i);
}

int main(int argc, char* argv[]) {
    int i = 1;

    if(argc != 2) {
        std::cout << "Uso correto: " << argv[0] << " <ID do usuario>" << std::endl;
        return 1;
    }

    std::string id = argv[1];

    // Realiza a primeira requisição CURL para obter os dados da API
    std::string link = "https://kitsu.io/api/edge/users/"+ id +"/library-entries?page[limit]=20&page[offset]=0";
    std::string responseData = getCurl(link);

    // Lê os dados JSON recebidos e extrai as informações relevantes para cada anime
    readerJson(responseData, i);

    // Fecha o arquivo de texto após a escrita dos dados
    txt.close();

    return 0;
}

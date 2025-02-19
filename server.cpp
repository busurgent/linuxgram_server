#include <boost/asio.hpp>
#include <iostream>
#include <optional>
#include <nlohmann/json.hpp>
#include <postgresql/libpq-fe.h>

class session: public std::enable_shared_from_this<session> {
  public:
    session(boost::asio::ip::tcp::socket&& socket) :
        socket(std::move(socket)) {}

    void read_from_socket() {
        boost::asio::async_read_until(
            socket,
            streambuf,
            '\n',
            [self = shared_from_this()](
                boost::system::error_code error,
                std::size_t bytes_transferred) {
                    self->data = std::string(boost::asio::buffers_begin((self->streambuf).data()), boost::asio::buffers_end((self->streambuf).data()));
                    std::cout << "read" << std::endl;
                    self->query_analyzer();
                    // self->streambuf.consume(self->streambuf.size());
                    // std::cout << data;

                    self->write_to_socket();


            });
    }

    void query_analyzer()
    {
        std::cout << data << std::endl;
        nlohmann::json j = nlohmann::json::parse(data);
        json.clear();

        const char* conninfo = "dbname=linuxgram user=ilya hostaddr=127.0.0.1 port=5432 password=qwerty";
        PGconn* conn = PQconnectdb(conninfo);
        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Ошибка подключения к базе данных: " << PQerrorMessage(conn) << std::endl;
            PQfinish(conn);
        }
        // if (j["type"] == "register") {
            char format[1024];
            std::cout << "bd_query1" << std::endl;
            std::string login = j["username"];
            std::string password = j["password"];

            std::cout << "bd_query2" << std::endl;
            
            snprintf(format, 1024, "INSERT INTO users (username, password, token) VALUES (\'%s\', \'%s\', \'%s\');", login.c_str(), password.c_str(), "88005553535");
            PGresult* res = PQexec(conn, format);
        // }

        // else if (j["type"] == "login") {

        // }

        // else if (j["type"] == "search") {

        // }

        // else if (j["type"] == "select_user") {

        // }

        // else if (j["type"] == "message") {

        // }

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::cerr << "Ошибка выполнения запроса: " << PQerrorMessage(conn) << std::endl;
            PQclear(res);
            PQfinish(conn);
            return;
        }

        PQclear(res);
        PQfinish(conn);

        return;
    }

    void write_to_socket()
    {
        boost::asio::async_write(
            socket,
            streambuf, 
            [self = shared_from_this()](
                boost::system::error_code error,
                    std::size_t bytes_transferred){
                        self->read_from_socket();
                    });
    }

  private:
    boost::asio::ip::tcp::socket socket;
    boost::asio::streambuf streambuf;
    std::string data;
    nlohmann::json json;
};

class server {
  public:
    server(boost::asio::io_context& io_context, std::uint16_t port) :
        io_context(io_context),
        acceptor(
            io_context,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {}

    void async_accept() {
        socket.emplace(io_context);

        acceptor.async_accept(*socket, [&](boost::system::error_code error) {
            std::make_shared<session>(std::move(*socket))->read_from_socket();
            async_accept();
        });
    }

  private:
    boost::asio::io_context& io_context;
    boost::asio::ip::tcp::acceptor acceptor;
    std::optional<boost::asio::ip::tcp::socket> socket;
};

int main() {
    boost::asio::io_context io_context;
    server srv(io_context, 53861);
    srv.async_accept();
    io_context.run();
    return 0;
}
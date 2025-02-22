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
                if (!error) {
                    self->data = std::string(boost::asio::buffers_begin(self->streambuf.data()), 
                                             boost::asio::buffers_end(self->streambuf.data()));
                    self->query_analyzer();
                    self->streambuf.consume(bytes_transferred);
                    self->write_to_socket();
                } else {
                    std::cerr << "Ошибка чтения: " << error.message() << std::endl;
                    // Можно закрыть соединение или предпринять другие действия
                }
        });
}

    void query_analyzer() {
    try {
        std::cout << data << std::endl;
        auto j = nlohmann::json::parse(data);
        const char* conninfo = "dbname=linuxgram user=ilya hostaddr=127.0.0.1 port=5432 password=qwerty";
        PGconn* conn = PQconnectdb(conninfo);
        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Ошибка подключения к базе данных: " << PQerrorMessage(conn) << std::endl;
            PQfinish(conn);
            return;
        }

        if (j["type"] == "register") {
            std::string login = j["username"];
            std::string password = j["password"];
            const char* paramValues[2] = { login.c_str(), password.c_str() };

            PGresult* res = PQexecParams(
                conn,
                "INSERT INTO users (username, password, token) VALUES ($1, $2, '88005553535')",
                2,       // Количество параметров
                nullptr, // Типы параметров (nullptr → определяются автоматически)
                paramValues,
                nullptr, // Длины параметров
                nullptr, // Форматы параметров
                0        // Формат результата (0 = текст)
            );

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                std::cerr << "Ошибка выполнения запроса: " << PQerrorMessage(conn) << std::endl;
            }

            PQclear(res);
        }
        json.clear();
        json["status"] = "OK";
        json["user_id"] = 0;
        std::ostream os(&streambuf1);
        os << json.dump();

        PQfinish(conn);
    } catch (const std::exception& e) {
        std::cerr << "Ошибка обработки JSON или SQL-запроса: " << e.what() << std::endl;
    }
}

    void write_to_socket() {
    boost::asio::async_write(
        socket,
        streambuf1,
        [self = shared_from_this()](
            boost::system::error_code error,
            std::size_t bytes_transferred) {
                if (!error) {
                    self->read_from_socket();
                } else {
                    std::cerr << "Ошибка записи: " << error.message() << std::endl;
                    // Закрыть соединение или обработать ошибку
                }
        });
}

  private:
    boost::asio::ip::tcp::socket socket;
    boost::asio::streambuf streambuf;
    boost::asio::streambuf streambuf1;
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
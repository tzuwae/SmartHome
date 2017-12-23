import multiprocessing
import socket
import pymysql
import time

#Database setup
dbServer="localhost"
dbUser="user"
dbPassword="0000"
dbInuse="test"
dbTable="espdb"
db = pymysql.connect(dbServer, dbUser, dbPassword, dbInuse)
cursor = db.cursor()
todolist = "Hello world."
txmessage =""

def handle(connection, address):
    import logging
    logging.basicConfig(level=logging.DEBUG)
    logger = logging.getLogger("process-%r" % (address,))
    try:
        logger.debug("Connected %r at %r", connection, address)
        while True:
            connection.settimeout(1)
            data = connection.recv(1024)

            if data == "":
                logger.debug("Socket closed remotely")
                break
            if len(data) < 2:
                logger.debug("Socket closed remotely")
                break

            logger.debug("Received data %r", data)
            #ClientTX = str(data)
            #connection.sendall(ClientTX[2:4:1].encode())
            #connection.sendall("AuthOK!".encode()+data)
            txmessage = time.strftime("%I:%M %p%a %b %d %Y", time.localtime())
            connection.sendall(txmessage.encode() )
            #connection.sendall(str((time.strftime("%a %b %d %H:%M %Y", time.localtime())) + str(todolist)).encode())
            data=data.strip()
            data=str(data.decode())

            logger.debug("Sent data")
            try:
                sql = """SELECT * from espdb WHERE MAC='""" + str(data) + "';"
                print(sql)
                cursor.execute(sql)
                db.commit()
                results = cursor.fetchall()
                if results == ():
                    sql = """INSERT INTO espdb VALUES ('""" + str(data) + """',""" + """'""" + str(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())) + """');"""
                    print("New client:" + str(data))
                else:
                    sql = """UPDATE espdb SET Time='""" + str(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())) + """' WHERE MAC='""" + str(data) + """';"""

                print(sql)
                cursor.execute(sql)
                db.commit()
            except:
                db.rollback()
                print("Error: unable to fetch data")
            logger.debug("Update Database")
            break

    except socket.timeout:
        logger.exception("timeout")
    except:
        logger.exception("Problem handling request")

    finally:
        logger.debug("Closing socket")
        connection.close()


class Server(object):
    def __init__(self, hostname, port):
        import logging
        self.logger = logging.getLogger("server")
        self.hostname = hostname
        self.port = port

    def start(self):
        self.logger.debug("listening")
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind((self.hostname, self.port))
        self.socket.listen(1)

        while True:
            conn, address = self.socket.accept()

            self.logger.debug("Got connection")
            process = multiprocessing.Process(target=handle, args=(conn, address))
            process.daemon = True
            process.start()
            self.logger.debug("Started process %r", process)


if __name__ == "__main__":
    import logging
    logging.basicConfig(level=logging.DEBUG)
    server = Server("0.0.0.0", 12345)

    try:
        logging.info("Listening")
        server.start()
    except:
        logging.exception("Unexpected exception")
    finally:
        logging.info("Shutting down")
        for process in multiprocessing.active_children():
            logging.info("Shutting down process %r", process)
            process.terminate()
            process.join()
    logging.info("All done")
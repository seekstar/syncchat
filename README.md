# syncchat

#### Description

An IM software. Just for fun.

gitee: <https://gitee.com/searchstar/syncchat>

github: <https://github.com/searchstar2017/syncchat>

#### Software Architecture

The software is based on C/S architecture. The communicate between the server and the client is based on SSL. The passwords of users are hashed with salt.

The software is developed on linux. But it should be easy to adapt to windows because all components used are cross-platform.

#### Installation

##### server

1. Install dependencies

    - boost
    - openssl
    - unixodbc

2. cd to the root of the project
3. Compile server

    ```shell
    mkdir build
    cd build
    cmake ..
    make
    ```

    Then the executable of server is in the directory "build".

4. Configure odbc data source

    The name of data source should be "syncchatserver". Here is an article about the configuration of odbc data source for mariadb on linux:
    <https://blog.csdn.net/qq_41961459/article/details/105142898>

5. Prepare certificates for server

    You need to put server.pem and dh2048.pem in the same directory as the executable of server.
    You can find the demo certificate in the directory "demo".
    Here is an article about how to generate ssl certificates on linux:
    <https://blog.csdn.net/qq_41961459/article/details/105720464>

6. Run server

    ```shell
    ./server 5188
    ```

##### client

1. Install dependencies

    - boost
    - openssl
    - unixodbc
    - sqlite3
    - Qt5

2. cd to client directory

3. Compile client

    ```shell
    qmake client.pro
    make in ../build-client-unknown-Debug
    ```

    Then the executable of client is in the current client directory.

4. Prepare certificate for client

    You need to put ca.pem in the same directory as the executable of client.
    You can find the demo ca.pem in the directory.

5. Run client

    ```shell
    ./client
    ```

#### Features

- Signup
- Login
- Add friend
- Delete friend
- Search for user ID by user name
- Private message
- Create group
- Join group
- Group message
- Moments and comments

#### User Interface

![Main window](https://wx2.sbimg.cn/2020/05/11/mainWindow.png)

#### TODO

- Delete friend offline.
- Delete group.
- Synchronize private messages and group messages between the server and the client.
- Support adding friends offline.
- Use local db as cache in "moments".
- Replace QDialog with QMainWindow.
- Support avatar.
- Support inserting pictures in message content and moment content.
- Support transferring files by p2p.
- Support windows.

#### Contribution

1. Fork the repository
2. Create Feat_xxx branch
3. Commit your code
4. Create Pull Request

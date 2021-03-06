//
//  HubMain.cpp
//  Hub
//
//  Created by YOOGYOUNGYOON on 2016. 4. 9..
//  Copyright © 2016년 YOOGYOUNGYOON. All rights reserved.
//

#include "HubMain.h"
#include "pClient.h"
#include "MQueue.h"

map<string, string> cfg_info_map;
HubMain g_app;
comm_log c_log;

int HubMain::initialize() {
    //log file location
    c_log.comm_log_init(g_app.getProcessName(), c_log.getCurTime());
    
    //process check interval
    p_check_interval = 10;
    
    //config file reading
    char* env_file;
    string cfg_location;
    ifstream cfg_fd;
    
    env_file = getenv("PROCESS_CFG");
    if (env_file == NULL) {
        cerr<<"Not found env variable. "<<endl;
    }else{
        cfg_location = string(env_file);
        cout<<"env: "<<env_file<<endl;
    }
    
    
    cfg_fd.open (cfg_location);
    
    string line;
    string word;
    string a, key, value;
    if(cfg_fd.is_open()){
        while (getline(cfg_fd, line)) {
            for (int i =0; i<line.size(); i++) {
                a=line.at(i);
                if (a=="#") break;
                if (a==":") {
                    key=line.substr(0,i-1);
                    value = line.substr(i+2, line.size());
                    break;
                }
            }
            cfg_info_map[key] = value;
        }
    }else{
        cerr<<"file is not exist!"<<endl;
    }
    cfg_fd.close();
    
    max_fd = 0 ;
    port_num  = atoi(cfg_info_map["HUB_PORT"].c_str());
    return 0;
}

int HubMain::process() {
    std::cout << "HubMain::process\n";
    while(1)
    {
        if(server_fd<1) {
            ServerListen();
            max_fd = max(max_fd, server_fd);
        }
        fd_set fds;
        FD_ZERO(&fds);
        if (server_fd>1) {
            FD_SET(server_fd, &fds);
        }
        
        list<pClient*>::iterator itr, itrPrev;
        for(itr = client_list.begin(); (itrPrev = itr) != client_list.end();) {
            pClient* tmp_client = *itr;
            int c_fd = tmp_client->getSocket();
            itr++;
            if(c_fd){
                FD_SET(c_fd, &fds);
            }else{
                client_list.erase(itrPrev);
                tmp_client->stop();
                delete tmp_client;
            }
        }
        int state = ::select(max_fd+1, &fds, NULL, NULL, NULL);
        if (state<=0) {
            cout<<"select error occur!"<<endl;
            continue;
        }else{
            if(FD_ISSET(server_fd, &fds)) {
                ClientAccept();
            }
            for(itr = client_list.begin(); (itrPrev = itr) != client_list.end();itr++) {
                pClient* tmp_client = *itr;
                
                int c_fd = tmp_client->getSocket();
                if (FD_ISSET(c_fd, &fds)) {
                    ssize_t msg_size = 0;
                    char* buffer = NULL;
                    msg_size = tmp_client->read(buffer);  // read 하고
                    
                    tmp_client->postMessage( msg_size, (unsigned long) buffer);
                    
                    if(msg_size<0) {
                        tmp_client->close();
                    }
                }
            }
        }
    }
    
    return 0;
    
}

int HubMain::stop() {
    c_log.write_log(10,"Hub Main Stop");
    return 0;
}

int HubMain::ServerListen() {
    struct sockaddr_in server_addr;
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {// 소켓 생성
        printf("Server : Can't open stream socket\n");
        exit(0);
    }
    memset(&server_addr, 0x00, sizeof(server_addr));
    //server_Addr 을 NULL로 초기화
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port_num);
    //server_addr 셋팅
    
    if(::bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <0)
    {//bind() 호출
        printf("Server : Can't bind local address.\n");
        exit(0);
    }
    
    if(::listen(server_fd, 5) < 0)
    {//소켓을 수동 대기모드로 설정
        printf("Server : Can't listening connect.\n");
        exit(0);
    }
    return 0;
}

int HubMain::ClientAccept() {
    socklen_t len;
    struct sockaddr_in client_addr;
    int client_fd=0;
    client_fd = ::accept(server_fd, (struct sockaddr *)&client_addr, &len);
    if(client_fd < 0)
    {
        printf("Server: accept failed.\n");
        exit(0);
    }
    
    // Client Class
    pClient *client = new pClient;
    client->initialize(client_fd);
    client_list.push_back(client);
    client->run(3);
    max_fd = max(max_fd, client_fd);

    
    return 0;
}

list<pClient*> & HubMain::getClientList(){
    return client_list;
}



int main(int argc, const char * argv[]) {
    if(g_app.initialize()) {
        cerr<<"Initialize Error!"<<endl;
        exit(EXIT_SUCCESS);
    };
    g_app.process();
    g_app.stop();
    
    
    
    return 0;
}

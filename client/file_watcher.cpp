
#include "file_watcher.h"
#include <chrono>

// Keep a record of files from the base directory and their last modification time
FileWatcher::FileWatcher(shared_map<Node> *remote_snapshot,shared_map<bool> * pending_operation, const std::string& path_to_watch, std::chrono::duration<int, std::milli> delay) : path_to_watch{path_to_watch},
                                                                                                                                       delay{delay}, remote_snapshot{remote_snapshot} ,pending_operation{pending_operation}
{
    std::cout << BLUE << "[FW CREATING INDEXES] " << RESET << std::endl;
    for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch))
    {
        Node tmp = createNode(file);
        std::cout << BLUE << "[HASHING] "<< RESET << file.path().relative_path().string() << std::endl;
        paths_.insert({file.path().relative_path().string(), tmp});
    }
    std::cout << "----------------------------" << std::endl;
}

// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
void FileWatcher::start(const std::function<void(Node, FileStatus)> &action)
{
    std::cout << BLUE << "[START MONITORING]" << RESET << std::endl;

    while (running_)
    {
        std::this_thread::sleep_for(delay);
        std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
        std::unordered_map<std::string,bool> files_deleted;
        _remote_snapshot = remote_snapshot->get_map();
        std::filesystem::file_time_type time;
        long int current_file_last_write_time;
        // check se un file e' stato cancellato sul fs locale
        auto it = paths_.begin();
        while (it != paths_.end())
        {
            if (!std::filesystem::exists(it->first))
            {
                action(it->second, FileStatus::erased); // it->first chiave
                files_deleted.insert({it->first,true}); // mi serve per controllare che il file appena cancellato non emetta un "MISSING"
                it = paths_.erase(it);
            }
            else
            {
                it++;
            }
        }
        // Check if a file was created or modified
        for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch))
        {

            if (!file.is_directory() && !is_file_being_copied(file.path().string()))
            {
                //cout<<"["<<file.path().string() <<"]"<<endl;
                time = std::filesystem::last_write_time(file);
                current_file_last_write_time = to_timestamp(time);
                bool isPendingOperation=pending_operation->exist(file.path().relative_path().string());



                // FILE  ESISTENTE SUL REPO REMOTO e anche sul fs locale MA NON SU PATHS (probABILE RITORNO DELLA GET)
                if(!local_snapshot_contains(file.path().string()) && remote_snapshot_contains(file.path().string())){


                   // Node tmp = createNode(file);
                   // paths_.insert({file.path().string(), tmp});
                    paths_.insert({file.path().string(), _remote_snapshot.at(file.path().string().erase(0, 2))}); // TODO erase  0,2 toglie '..' solito discorso..

                    paths_[file.path().string()].setLastWriteTime(current_file_last_write_time);
                }


                // CREATION file creato sul fs locale
                if (!local_snapshot_contains(file.path().string()) && !remote_snapshot_contains(file.path().string()))
                {
                    Node tmp = createNode(file);
                    paths_.insert({file.path().string(), tmp});
                    paths_[file.path().string()].setLastWriteTime(current_file_last_write_time);
                    action(tmp, FileStatus::created);
                }

                else
                {

                    // FILE NON ESISTENTE SUL REPO REMOTO
                    if (!remote_snapshot_contains(file.path().string()))
                    {
                        Node tmp;
                        if(isPendingOperation){
                            // operation on this file already run, no hash
                             tmp = createNodeNoHash(file);
                        }else{
                             tmp = createNode(file);
                        }
                        action(tmp, FileStatus::untracked);

                    }



                    // File modification
                    if (!areTsEqual(paths_[file.path().string()].getLastWriteTime() , current_file_last_write_time))
                    {
                        // probabile file modificato

                        std::string old_hash = paths_[file.path().string()].getLastHash();

                        std::string new_hash = Hasher::getSHA(file.path().string());
                        //cout<<"---"<<endl<<old_hash<<endl<<new_hash<<endl;
                        cout<<"--maybe modified check hash-"<<file.path().string()<<":"<<old_hash<<"@"<<new_hash<<endl;
                        if (old_hash != new_hash)
                        {   // se cambia l'hash la modifica e' veritiera
                            uintmax_t size = std::filesystem::file_size(file);
                            paths_[file.path().string()].setSize(size);
                            paths_[file.path().string()].setLastWriteTime(current_file_last_write_time);
                            paths_[file.path().string()].setLastHash(new_hash);
                            action(paths_[file.path().string()], FileStatus::modified);
                        }
                    }
                }
            }
        }

        // check se un file e' presente sul fs remoto ma non sul locale
        it = _remote_snapshot.begin();
        while (it != _remote_snapshot.end()) //  /my_sync_folder/file1.txt
        {
            //TODO .. work only if the directory is up one level
            if (!local_snapshot_contains(".." + it->first))  // c:/documenti/francesco +  /my_sync_folder/file1.txt
            {
                auto el = files_deleted.find(".."+it->first);
                if(el==files_deleted.end())// if file is not already deleted
                    action(it->second, FileStatus::missing); // chiama la get file

            }
            it++;
        }
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "(fw wake up for "<<duration.count()  <<"ms)" << std::endl;
    }

}



bool FileWatcher::is_file_being_copied(const boost::filesystem::path &filePath){
    boost::filesystem::fstream fileStream(filePath, std::ios_base::in | std::ios_base::binary);

    if(fileStream.is_open())
    {
        //We could open the file, so file is good
        //So, file is not getting copied.
        return false;
    }
    else
    {
        fileStream.close();
        cout<<"{ "<<filePath<<" } is being copied,skipping scan."<<endl;
        return true;
        //Wait, the file is currently getting copied.
    }
}

Node FileWatcher::createNodeNoHash(const std::filesystem::directory_entry &file)
{
    if (!std::filesystem::is_directory(file))
    {
        uintmax_t size = std::filesystem::file_size(file);
        auto time = std::filesystem::last_write_time(file);
        Node tmp(file.path().relative_path().string(), false, size, to_timestamp(time));
        return tmp;
    }
    else
    {
        auto time = std::filesystem::last_write_time(file);
        Node tmp(file.path().relative_path().string(), true, "", 0, to_timestamp(time));
        return tmp;
    }
}

Node FileWatcher::createNode(const std::filesystem::directory_entry &file)
{
    if (!std::filesystem::is_directory(file))
    {
        uintmax_t size = std::filesystem::file_size(file);
        std::string hash = Hasher::getSHA(file.path().string());
        auto time = std::filesystem::last_write_time(file);
        Node tmp(file.path().relative_path().string(), false, hash, size, to_timestamp(time));
        return tmp;
    }
    else
    {
        auto time = std::filesystem::last_write_time(file);
        Node tmp(file.path().relative_path().string(), true, "", 0, to_timestamp(time));
        return tmp;
    }
}
bool FileWatcher::areTsEqual(long int ts1,long int ts2){
    if(std::abs(ts1-ts2)>20){
        return false;
    }return true;
}

// Check if "paths_" contains a given key
// If your compiler supports C++20 use paths_.contains(key) instead of this function
bool FileWatcher::local_snapshot_contains(const std::string &key)
{
    auto el = paths_.find(key);
    return el != paths_.end();
}

bool FileWatcher::remote_snapshot_contains(const std::string &key)
{
    string tmp = key;
    // TODO erase 0 2 toglie '..'
    auto el = _remote_snapshot.find(tmp.erase(0, 2));
    return el != _remote_snapshot.end();
}
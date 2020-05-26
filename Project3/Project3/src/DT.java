import java.util.ArrayList;

public class DT {
  public class Entry{
      public int fileID;
      public int index; //the first block of the file
      public int size;
      
      public Entry(int fileID, int index, int size){
          this.fileID = fileID;
          this.index = index;
          this.size = size;
      }
      @Override
      public boolean equals(Object obj) {
          if(obj instanceof Entry)
            return ((Entry)obj).fileID == this.fileID;
          else
            return false;
      }

  }  
  public ArrayList<Entry> files = new ArrayList<Entry>();
  //Takes a fileID, fileLength in terms of bytes, an index and BLOCK_SIZE as parameters.
  //Adds the first block of the file as index and required number of blocks as size 
  //to the files 
  public void addFile(int fileID, int fileLength, int index, double BLOCK_SIZE) {
    Entry newFile = new Entry(fileID, index, (int)Math.ceil((double)fileLength/BLOCK_SIZE));
    files.add(newFile);
  }
  //Takes a fileID, blocks and index as parameter and overwrites the information
  //of the file whose id is FileID with new information
  public void updateFile(int fileID, int blocks, int index){
    for(Entry entry : files){
        if(entry.fileID == fileID){
            entry.size = blocks;
            entry.index = index;
        }
     }
  }
  //Checks whether a file with given fileID is in DT
  public boolean inDT(int fileID){
    for(Entry entry : files){
        if(entry.fileID == fileID){
           return true;
        }
    }
    return false;
  }
  //Returns the information in DT associated with the given fileID
  public int[] getFileInfo(int fileID){
      int[] info = new int[3];
      for(Entry entry : files){
         if(entry.fileID == fileID){
             info[0] = entry.fileID;
             info[1] = entry.index;
             info[2] = entry.size;
         }
      }
      return info;
  } 
  public void removeFile(int fileID){
      files.remove(new Entry(fileID, -1, -1));
  }
  public String printFiles(){
      String filesIDs = "";
      for(Entry file: files){
          filesIDs += " "+file.fileID;
      }
      return filesIDs;
  }
  public ArrayList<Entry> getFiles(){
      return files;
  }
}
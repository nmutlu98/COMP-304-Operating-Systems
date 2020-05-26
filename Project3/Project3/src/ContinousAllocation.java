import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Scanner;

public class ContinousAllocation extends Allocation {

    @Override
    //initializes necessary fields
    public void init() {
        currentFileID = 1;
        dt = new DT();
        fillZeros(0, disk.length - 1);
        creationRejected = 0;
        extensionRejected = 0;
        totalCreation = 0;
        totalExtension = 0;
        totalAccess = 0;
        totalShrinking = 0;
        totalTime = 0;
        totalCreationTime = 0;
        totalExtensionTime = 0;
        totalAccessTime = 0;
        totalShrinkingTime = 0;
    }

    @Override
    //Finds the first index of the hole that the file can fit
    //If there is no such hole, returns false. 
    //Otherwise, fills the appropriate parts of the disk, adds file information to DT
    // and returns true
    public boolean createFile(int fileID, int fileLength) {
        int index = findFirstFit(fileLength);
        if(index == -1){
            return false;
        }else{
            int blocks = (int)Math.ceil((double)fileLength/BLOCK_SIZE);
            for(int i = index; i<index+blocks; i++)
                disk[i] = 1;
            dt.addFile(fileID, fileLength, index, BLOCK_SIZE);
            return true;
        }
    }

    @Override
    public int access(int fileID, int byte_offset) {
        if(dt.inDT(fileID)){
            int[] fileInfo = dt.getFileInfo(fileID);
            int index = fileInfo[1];
            //if requested byte is not in the file, returns -1
            if(byte_offset > fileInfo[2]*BLOCK_SIZE)
                return -1;
            double relativeBlock = Math.ceil((double)byte_offset/BLOCK_SIZE);
            return index+(int)relativeBlock-1;
        }
        return -1;
    }

    @Override
    //finds the last index of the file in the disk
    //then makes shrinking amount of cells zero
    //if size of the file is 0, deletes it from DT
    //otherwise just updates the file information
    public void shrink(int fileID, int shrinking) {
        if(dt.inDT(fileID)){
            int[] fileInfo = dt.getFileInfo(fileID);
            int index = fileInfo[1];
            int size = fileInfo[2];
            if(shrinking <= size){
                int lastIndex = index+size-1;
                for(int i = lastIndex; i>lastIndex-shrinking; i--)
                    disk[i] = 0;
                if(size-shrinking <= 0)
                    dt.removeFile(fileID);
                else
                    dt.updateFile(fileID, size-shrinking, index);
            }
        }
    }

    @Override
    //If there is enough space at the end of the file, it extends it by putting 
    //ones to appropriate disk indices. Otherwise, it defragments the disk and tries to move
    //file to the end of the disk if there is enough space. After moving, if there is enough
    //space at the end of the file now, it extends the file. 
    public boolean extend(int fileID, int extension) {
        if(dt.inDT(fileID)){
            int[] fileInfo = dt.getFileInfo(fileID);
            int index = fileInfo[1];
            int size = fileInfo[2];
            int lastIndex = index+size-1;
            boolean enoughSpace = true;
            for(int i = lastIndex+1; i<=lastIndex+extension; i++){
                if(i >= disk.length || disk[i] != 0){
                    enoughSpace = false;
                    break;
                }
            }
            if(enoughSpace){
                for(int i = lastIndex+1; i<=lastIndex+extension; i++)
                    disk[i] = 1;
                dt.updateFile(fileID, size+extension, index);
                return true;
            }
            defragment();
            //update fileInfo after defragmentation
            fileInfo = dt.getFileInfo(fileID);
            index = fileInfo[1];
            size = fileInfo[2];
            boolean success = moveFile(fileID, index, size, freeIndexFromEnd());
            //if file is moved to the end, it calls defragment again to fill the hole
            //caused by the relocation of the file
            if(success){
                defragment();
                //update fileInfo after defragmentation
                fileInfo = dt.getFileInfo(fileID);
                index = fileInfo[1];
                size = fileInfo[2];
                //if there is no enough space at the end, extension is rejected
                if(countFreeSpace(index+size) < extension)
                    return false;
                for(int i = index+size; i<index+size+extension; i++)
                    disk[i] = 1;
                dt.updateFile(fileID, size+extension, index);
                return true;
            }
            
                
        }
          return false;         
    }
    @Override
    //Given a filename, does the operations in it. Writes output to "results.txt"
    public void operateOnFile(String filename) {
        init();
        int bsIndex = filename.indexOf("_", 7);
        BLOCK_SIZE = Integer.parseInt(filename.substring(6, bsIndex));
        try {
            File file = new File("Project3/io/" + filename);
            Scanner cursor = new Scanner(file);
            long startTime = System.currentTimeMillis();
            while (cursor.hasNextLine()) {
                String line = cursor.nextLine();
                String operation = line.substring(0, line.indexOf(":"));
                if (operation.equals("c")) {
                    create(line);
                }else if(operation.equals("a")){
                    accessFile(line);
                }else if(operation.equals("sh")){
                    shrinkFile(line);
                }else{
                    extendFile(line);
                }
                long endTime = System.currentTimeMillis();
                totalTime = endTime-startTime;
           }
            cursor.close();
        } catch (Exception e) {
            e.printStackTrace();
        } 
        try {
            FileWriter results = new FileWriter("results.txt", true);
            results.write("Continous Allocation File Name: "+filename);
            results.write("\n");
            results.write("Number of creation rejected: "+creationRejected+" Total creation: "+totalCreation+" Number of extension rejected: "+extensionRejected+" Total Extension: "+totalExtension);
            results.write("\n");
            results.write("Average time: "+(totalTime/(totalCreation+totalAccess+totalExtension+totalShrinking)));
            results.write("\n");
            if(totalCreation != 0)
                results.write("Average creation: "+(totalCreationTime/totalCreation));
            results.write("\n");
            if(totalExtension != 0)
                results.write("Avearage extension: "+(totalExtensionTime/totalExtension));
            results.write("\n");
            if(totalShrinking != 0)
                results.write("Avearage shrinking: "+(totalShrinkingTime/totalShrinking));
            results.write("\n");        
            if(totalAccess != 0)
                results.write("Avearage accession: "+(totalAccessTime/totalAccess));
            results.write("\n");
            results.write("-------------------------------------------------------------");
            results.write("\n");
            results.close();
                    
          } catch (IOException e) {
            System.out.println("An error occurred.");
            e.printStackTrace();
          } 
       

    }
    //finds the beginning index of the empty part of the disk from the end
    public int freeIndexFromEnd(){
        for(int i = disk.length-1; i>=0; i--){
            if(disk[i] == 1)
                return i+1;
        }
        return -1;
    }
    //Takes an index as parameter and finds the free cells after this index
    //including the index itself
    public int countFreeSpace(int index) {
        int total = 0;
        for(int i = index; i<disk.length; i++){
            if(disk[i] == 0)
                 total++;
        }
        return total;
     }
     //Takes an index as parameter and finds the first filled cell after it
    public int firstFilledBlock(int index){
        for(int i = index + 1; i<disk.length; i++){
            if(disk[i] != 0)
                return i;
        }
        return -1;
    }
    //Given the length of the file in terms of bytes, finds the first hole that the 
    //file can fit. If the first search is unsuccesfull, defragments the disk and
    //searches again.
    public int findFirstFit(int length){
        int index = searchInDisk(length);
        if(index == -1){
            defragment();
             index = searchInDisk(length);
        }
       
        return index;
        
    }
    //Given length of a file in terms of bytes, returns the start index of a hole
    //that the file can fit into. If there is no such hole, returns -1. 
    public int searchInDisk(int length){
        int i = 0;
        int index = -1;
        int numBlocks = (int)Math.ceil((double)length/BLOCK_SIZE);
        while(i<disk.length){
            boolean found = true;
            for(int j = i; j< i+numBlocks; j++){
                if(numBlocks>disk.length || (j<disk.length && disk[j] != 0) || j>=disk.length ){
                    found = false;
                    i = j;
                    break;
                }
            }
            if(found){
                index = i;
                return index;
            }
            i++;
        }
        return index;
    }
    //Takes an index as parameter and finds the first empty cell before that index
    public int findFirstFreeCellBefore(int index){
        for(int i = 0; i<index; i++){
            if(disk[i] == 0)
                return i;
        }
        return -1;
    }
    public void defragment(){
        for(DT.Entry file : dt.getFiles()){
            int free = findFirstFreeCellBefore(file.index);
            if(free != -1){
                int size = file.size;
                int index = file.index;
                dt.updateFile(file.fileID, size, free);
                while(size > 0){
                    swap(index, free);
                    index++;
                    size--;
                    free = findFirstFreeCellBefore(index);
                }
            }
        }
    }
    public void swap(int i, int j){
        int singleBlock = disk[i];
        disk[i] = disk[j];
        disk[j] = singleBlock;
    }
    //Takes fileID, the start index and size of the file whose id is fileID and 
    //an index of an empty block(freePartLocation).
    //Returns true if moving file to the freePartLocation is succesfull. Otherwise
    //returns false
    public boolean moveFile(int fileID, int index, int size, int freePartlocation) {// size burda block
        int space = countFreeSpace(freePartlocation);
        int newIndex = freePartlocation;
        //if space after the given index is enough for the file, moves file
        if(space >= size && freePartlocation != -1){
            while(size > 0){
                swap(index, freePartlocation);
                index++;
                freePartlocation++;
                size--;
            }
            //updates the file info as the index of the file changed
            dt.updateFile(fileID, dt.getFileInfo(fileID)[2], newIndex);
            return true;
        }
        return false;
    }
}
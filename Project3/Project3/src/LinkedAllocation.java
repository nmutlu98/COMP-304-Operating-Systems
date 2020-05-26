import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Scanner;

public class LinkedAllocation extends Allocation {
    
    private int numBlocksFAT; //stores the number of blocks that will be allocated to FAT from the disk
    public static int lastIndex; //the index just before the part that is allocated to FAT.
    public static FAT fat;

    @Override
    //Initializes necessary fields for linked allocation
    public void init() {
        dt = new DT();
        currentFileID = 1;
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
        fillZeros(0, disk.length-1);
        fat = new FAT(32768);

    }

    @Override
    //If required numBlocks is higher than the available
    //space rejects the create request. Otherwise adds new file information to DT.
    public boolean createFile(int fileID, int fileLength) {
        int numBlocks = (int)Math.ceil((double)fileLength/BLOCK_SIZE);
        if(freeSpace() < numBlocks){
            return false;
        }
        //Find an empty box which will be the first block of the file
        int i = findFirstEmptyBox();
        //Add new file information to dt
        dt.addFile(fileID, fileLength, i, BLOCK_SIZE);
        //fill this index in disk
        disk[i] = 1;
        numBlocks--;
        //find enough empty cell to put other blocks of the file
        while(numBlocks > 0){
           int j = findFirstEmptyBox();
           disk[j] = 1;
           numBlocks--;
           //add the links between the empty cells to fat as pointers to next block
           fat.addLink(i, j);
           i = j;

        }
        return true;
    }
   
    @Override
    public int access(int fileID, int byte_offset) {
        if(dt.inDT(fileID)){
            int block = (int)Math.ceil((double)byte_offset/BLOCK_SIZE);
            int[] fileInfo = dt.getFileInfo(fileID);
            int index = fileInfo[1];
            if(byte_offset > fileInfo[2]*BLOCK_SIZE)
                return -1;
            if(block == 1)
                return index;
            while(block > 1){
                index = fat.getFileInfo(index);
                block--;
            }
            return index;
        }
        return -1;
    }

    @Override
    public void shrink(int fileID, int shrinking) {
        if(dt.inDT(fileID)){
            //Gets the current information of file
            int[] fileInfo = dt.getFileInfo(fileID);
            int start = fileInfo[1]; //stores the current block of the file
            int size = fileInfo[2];
            int next = -1; //stores the next block of the file
            int i = 0; //used as a counter to determine from which block to start shrinking
            int prev = -1; //stores the previous block of the file
            //if shrinking is larger than the size that the file have, it is rejected
            if(shrinking > size*BLOCK_SIZE){
                //forward the start pointer until the point where shrinking starts
                while(i < size-shrinking){
                    prev = start;
                    start = fat.getFileInfo(start);
                    i++;
                }
                //removes the link to the cell where shrinking starts.
                if(prev != -1)
                    fat.removeLink(prev);
                //Deletes the information in the related cells of the disk
                //Removes the links between the blocks in fat
                while(fat.getFileInfo(start) != -1){
                    disk[start] = 0;
                    next = fat.getFileInfo(start);
                    fat.removeLink(start);
                    start = next;
                }
                disk[start] = 0;
                //If file size became 0, remove the file
                if(size-shrinking <= 0)
                    dt.removeFile(fileID);
                else //update the file's info for new size
                    dt.updateFile(fileID, size-shrinking, fileInfo[1]);
            }
    }

    }

    @Override
    public boolean extend(int fileID, int extension) {
        if(dt.inDT(fileID)){
            //If there is not enough space, rejects the request
            if(freeSpace() < extension)
                return false;
            int index = dt.getFileInfo(fileID)[1];
            //Updates file information with the extended size
            dt.updateFile(fileID, dt.getFileInfo(fileID)[2]+extension, index);
            //finds where the last block is located
            while(fat.getFileInfo(index) != -1){
                index = fat.getFileInfo(index);
            }
            //Finds empty cells to extend the file.
            //Fills the new cell in the disk and adds links to the fat
            while(extension > 0){
                int i = findFirstEmptyBox();
                disk[i] = 1;
                fat.addLink(index, i);
                index = i;
                extension--;
            }
            return true;
        }
        return false;
    }

    @Override
    //Given a filename as parameter, does the operations in the file and prints the results
    //to the results.txt
    public void operateOnFile(String filename) {
        init();
        int bsIndex = filename.indexOf("_", 7);
        BLOCK_SIZE = Integer.parseInt(filename.substring(6, bsIndex));
        numBlocksFAT = (int)Math.ceil(32768*4/BLOCK_SIZE);
        lastIndex = disk.length-numBlocksFAT-1;
        try {
            File file = new File("Project3/io/"+filename);
            Scanner cursor = new Scanner(file);
            long startTime = System.currentTimeMillis();
            while(cursor.hasNextLine()){
                String line = cursor.nextLine();
                String operation = line.substring(0,line.indexOf(":"));
                if(operation.equals("c")){
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
            results.write("Linked Allocation File Name: "+filename+"\n");
            results.write("\n");
            results.write("Number of creation rejected: "+creationRejected+" Total creation: "+totalCreation+" Number of extension rejected: "+extensionRejected+" Total Extension: "+totalExtension+"\n");
            results.write("\n");
            results.write("Average time: "+(totalTime/(totalCreation+totalAccess+totalExtension+totalShrinking))+"\n");
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
    //Calculates and returns the total empty blocks in the disk
    public int freeSpace() {
        int a = 0;
        for(int i = 0; i<=lastIndex; i++){
            if(disk[i] == 0)
                a++;
        }
        return a;
    }
    //Finds the first empty cell starting from the beginning of the disk
    public int findFirstEmptyBox(){
        for(int i = 0; i<=lastIndex; i++){
            if(disk[i] == 0)
                return i;
        }
        return -1;
    }
    
}
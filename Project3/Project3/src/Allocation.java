//Allocation is an abstract class which includes common fields
//and common functions in LinkedAllocation and ContinousAllocation
public abstract class Allocation {

    public int[] disk = new int[32768];
    public int currentFileID; 
    int BLOCK_SIZE;
    DT dt;
    int creationRejected = 0; 
    int extensionRejected = 0;
    double totalCreation = 0;
    double totalExtension = 0;
    double totalAccess = 0;
    double totalShrinking = 0;
    long totalTime = 0; //totalTime spend on operations
    long totalCreationTime = 0; //total time spent on creating files
    long totalExtensionTime = 0; //total time spent on extending files
    long totalAccessTime = 0; //total time spent on accessing to files
    long totalShrinkingTime = 0; // total time spent on shrinking files

    //extracts the parameters from the line for createFile function
    public void create(String line) {
        int length = Integer.parseInt(line.substring(line.indexOf(":") + 1));
        long start = System.currentTimeMillis();
        boolean success = createFile(currentFileID, length);
        long end = System.currentTimeMillis();
        totalCreationTime += end - start;
        if(!success)
            creationRejected++;
        totalCreation++;
        currentFileID++;
    }
    //extracts the parameters from the line for shrink function
    public void shrinkFile(String line){
        String remLine = line.substring(line.indexOf(":")+1);
        int fileID = Integer.parseInt(remLine.substring(0,remLine.indexOf(":")));
        int shrinking = Integer.parseInt(remLine.substring(remLine.indexOf(":")+1));
        long start = System.currentTimeMillis();
        shrink(fileID, shrinking);
        long end = System.currentTimeMillis();
        totalShrinkingTime += end-start;
        totalShrinking++;
    }
    //extracts the parameters from the line for extend function
    public void extendFile(String line){
        String remLine = line.substring(line.indexOf(":")+1);
        int fileID = Integer.parseInt(remLine.substring(0,remLine.indexOf(":")));
        int extension = Integer.parseInt(remLine.substring(remLine.indexOf(":")+1));
        long start = System.currentTimeMillis();
        boolean success = extend(fileID, extension);
        long end = System.currentTimeMillis();
        totalExtensionTime += end-start;
        if(!success)
            extensionRejected++;
            totalExtension++;
    }
    //extracts the parameters from the line for access function
    public void accessFile(String line){
        String remLine = line.substring(line.indexOf(":")+1);
        int fileID = Integer.parseInt(remLine.substring(0,remLine.indexOf(":")));
        int offset = Integer.parseInt(remLine.substring(remLine.indexOf(":")+1));
        long start = System.currentTimeMillis();
        int block = access(fileID, offset);
        long end = System.currentTimeMillis();
        totalAccessTime += end-start;
        totalAccess++;
    }
    //fills the disk with zeros before experimenting with each file
    public void fillZeros(int low, int high){
        for(int i = low; i <= high; i++)
            disk[i] = 0;
    }
    public abstract boolean createFile(int fileID, int fileLength);
    public abstract int access(int fileID, int byte_offset);
    public abstract void shrink(int fileID, int shrinking);
    public abstract boolean extend(int fileID, int extension);
    //given a filename as a parameter, does the operations on the file 
    public abstract void operateOnFile(String filename);
    //initializes necessary parameters for experiments
    public abstract void init();
}
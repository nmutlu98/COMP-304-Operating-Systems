public class FAT {
    public int[] pointers;
    public FAT(int length){
        pointers = new int[length];
        for(int i = 0; i<pointers.length; i++)
            pointers[i] = -1;
    }
    public void addLink(int index, int value){
        pointers[index] = value;
    }
    //Takes an index as parameter and returns the block linked to that index
    public int getFileInfo(int index){
        return pointers[index];
    }
    //Takes an index as parameter and removes the link of it.
    public void removeLink(int index){
        pointers[index] = -1;
    }
}
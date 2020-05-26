public class Main {
    public static void main(String[] args) {
        LinkedAllocation linked = new LinkedAllocation();
        ContinousAllocation continous = new ContinousAllocation();
        linked.operateOnFile("input_8_600_5_5_0.txt");
        continous.operateOnFile("input_8_600_5_5_0.txt");
        linked.operateOnFile("input_1024_200_5_9_9.txt");
        continous.operateOnFile("input_1024_200_5_9_9.txt");
        linked.operateOnFile("input_1024_200_9_0_0.txt");
        continous.operateOnFile("input_1024_200_9_0_0.txt");
        linked.operateOnFile("input_1024_200_9_0_9.txt");
        continous.operateOnFile("input_1024_200_9_0_9.txt");
        linked.operateOnFile("input_2048_600_5_5_0.txt");
        continous.operateOnFile("input_2048_600_5_5_0.txt");
    }
}
#ifndef REG_TO_AFD_CONVERTER
#define REG_TO_AFD_CONVERTER

#include <fstream>
#include <string>
#include <set>
#include <stack>
#include <queue>
#include <deque>
#include <map>

class RegExToAFDConverter
{
private:
    struct Node
    {
        std::set<int> firstPos;
        std::set<int> lastPos;
        bool nullable;
        Node(std::set<int> firstPos, std::set<int> lastPos, bool n) : firstPos(firstPos), lastPos(lastPos), nullable(n)
        {
        }
        Node() = default;
    };

    struct AFDNode
    {
        std::set<int> pos;
        std::map<char, AFDNode *> nextNode;
        AFDNode() = default;
        AFDNode(std::set<int> pos) : pos(pos)
        {
        }
    };

    struct AFD
    {
        AFDNode *startNode;
        std::set<AFDNode *> finalNodes;
        AFD() : startNode(nullptr)
        {
        }
    };

    std::string filename;
    Node root;
    AFD afd;
    std::set<char> alphabet;
    std::map<int, char> indexToLetter;
    std::map<int, std::set<int>> followPosTable;
    std::string expression;
    std::string extendedPostfixExpression;

    void readAlphabet(std::ifstream &fin);
    void setAlphabet(const std::string &str);
    void readExpression(std::ifstream &fin);
    void setExtendedPostfixExpression();
    std::string convertExpressionToPostfixForm();
    static void handleOperator(char op, std::stack<char> &operatorsStack, std::queue<char> &postfixQueue);
    static bool compareOperatorPrecedence(char, char);
    static void addRemainingOperators(std::stack<char> &operatorsStack, std::queue<char> &postfixQueue);
    static std::string convertPostfixQueueToString(std::queue<char> &postfixQueue);
    void parseExtendedPostfixExpression();
    void traverseKleeneStarNode(std::stack<Node> & depthFirstStack);
    void traverseUnionNode(std::stack<Node> & depthFirstStack);
    void traverseConcatenationNode(std::stack<Node> & depthFirstStack);
    static std::set<int> getConcatenationNodeFirstPos(Node &right, Node &left);
    static std::set<int> getConcatenationNodeLastPos(Node &right, Node &left);
    void setAFD();
    std::map<char, std::set<int>> groupPositionsByLetter(std::set<int> &positions);
    std::set<int> getUnionOfPositionsFollowPos(std::set<int> &positions);
    void addNextStatesForCurrentState(AFDNode *currentState, std::set<AFDNode *> &breadthFirstQueue, std::deque<AFDNode *> &visitedNodes);
    static std::string positionsToString(const std::set<int> & positions);
    void writeStartAndFinalStates(std::ofstream & fout);
    void writeTransitions(std::ofstream & fout, AFDNode * currentNode, std::queue<AFDNode *> & breadthFirstQueue);


public:
    RegExToAFDConverter(const std::string &filename) : filename(filename)
    {
    }
    void parseFile();
    void buildAFD();
    void writeDOTFile(const std::string &outputFilename);
};

#endif
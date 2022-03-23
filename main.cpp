#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <stack>
#include <queue>
#include <deque>
#include <map>
#include <algorithm>
#include <sstream>
#include <tuple>

struct Node
{
    std::set<int> firstPos;
    std::set<int> lastPos;
    bool nullable;
    Node(std::set<int> firstPos, std::set<int> lastPos, bool n) : firstPos(firstPos), lastPos(lastPos), nullable(n)
    {
    }
};

struct AFDNode
{
    std::set<int> pos;
    std::map<char, AFDNode *> nextNode;
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

std::set<char> convertStringToAlphabet(const std::string &str);
std::string convertExprToPostfixForm(const std::string &expr);
std::tuple<std::map<int, char>, std::map<int, std::set<int>>, Node> buildTree(const std::string &postfixExpr);
AFD buildAFD(std::map<int, char> &indexToLetter, std::map<int, std::set<int>> &followPos, Node & root);
void writeDOTFile(AFD afd, const std::string &outputFilename);

int main()
{
    std::ifstream fin("input.txt");
    std::string alphabetString;

    std::getline(fin, alphabetString);
    auto alphabet = convertStringToAlphabet(alphabetString);
    alphabet.insert('#');

    std::string expression;
    std::getline(fin, expression);
    fin.close();

    std::cout << "Infix form: " << expression << std::endl;
    std::string postfixFormExpression = convertExprToPostfixForm(expression);
    std::cout << "Postfix form: " << postfixFormExpression << std::endl;

    std::string extendedExpression = postfixFormExpression + "#.";
    auto [indexToLetter, followPos, root] = buildTree(extendedExpression);
    for (auto indexToLetterPair : indexToLetter) {
        std::cout << indexToLetterPair.second << ": " << indexToLetterPair.first << std::endl;
    }

    for (auto pos : followPos) {
        std::cout << pos.first << ": ";
        for (int follow : pos.second) {
            std::cout << follow << " ";
        }
        std::cout << std::endl;
    }
    AFD afd = buildAFD(indexToLetter, followPos, root);
    writeDOTFile(afd, "afd.gv");
    return 0;
}

std::set<char> convertStringToAlphabet(const std::string &line)
{
    std::set<char> alphabet;
    size_t prev_pos = 0;
    size_t pos = line.find(' ');
    while (pos != std::string::npos)
    {
        alphabet.insert(line.substr(prev_pos, pos - prev_pos)[0]);
        prev_pos = pos + 1;
        pos = line.find(' ', pos + 1);
    }

    alphabet.insert(line.substr(prev_pos)[0]);

    return alphabet;
}

std::string convertExprToPostfixForm(const std::string &expression)
{
    std::stack<char> s;
    std::queue<char> q;
    std::string result;
    auto comparePrecedence = [](char op1, char op2)
    {
        if (op1 == '*')
            return true;
        if (op1 == '.' && op2 != '*')
            return true;
        if (op1 == '|' && op2 != '*' && op2 != '.')
            return true;
        return false;
    };

    for (char c : expression)
    {
        if (isalpha(c))
            q.push(c);
        else
        {
            if (c == '(' || !s.empty() && s.top() == '(')
                s.push(c);
            else if (c == ')')
            {
                while (s.top() != '(')
                {
                    q.push(s.top());
                    s.pop();
                }
                s.pop();
            }
            else
            {
                while (!s.empty() && comparePrecedence(s.top(), c))
                {
                    q.push(s.top());
                    s.pop();
                }
                s.push(c);
            }
        }
    }

    while (!s.empty())
    {
        q.push(s.top());
        s.pop();
    }

    result.reserve(q.size());
    while (!q.empty())
    {
        result += q.front();
        q.pop();
    }
    return result;
}

std::tuple<std::map<int, char>, std::map<int, std::set<int>>, Node> buildTree(const std::string &postfixExpr)
{
    std::stack<Node> s;
    std::map<int, char> indexToLetter;
    std::map<int, std::set<int>> followPos;
    int letterIdx = 1;
    for (char c : postfixExpr)
    {
        if (isalpha(c) || c == '#')
        {
            indexToLetter.insert({letterIdx, c});
            s.push(Node(std::set<int>({letterIdx}), std::set<int>({letterIdx}), false));
            letterIdx++;
        }
        else
        {
            if (c == '*')
            {
                Node top = s.top();
                s.pop();
                s.push(Node(top.firstPos, top.lastPos, true));
                for (int idx : top.lastPos)
                {
                    auto it = followPos.find(idx);
                    if (it == followPos.end())
                    {
                        followPos.insert({idx, top.firstPos});
                    }
                    else
                    {
                        it->second.insert(top.firstPos.begin(), top.firstPos.end());
                    }
                }
            }
            else if (c == '|')
            {
                Node right = s.top();
                s.pop();
                Node left = s.top();
                s.pop();
                std::set<int> firstPos;
                std::set<int> lastPos;
                std::set_union(left.firstPos.begin(), left.firstPos.end(), right.firstPos.begin(), right.firstPos.end(), std::insert_iterator<std::set<int>>(firstPos, firstPos.begin()));
                std::set_union(left.lastPos.begin(), left.lastPos.end(), right.lastPos.begin(), right.lastPos.end(), std::insert_iterator<std::set<int>>(lastPos, lastPos.begin()));
                s.push(Node(firstPos, lastPos, left.nullable || right.nullable));
            }
            else if (c == '.')
            {
                Node right = s.top();
                s.pop();
                Node left = s.top();
                s.pop();
                std::set<int> firstPos;
                std::set<int> lastPos;
                if (left.nullable)
                    std::set_union(left.firstPos.begin(), left.firstPos.end(), right.firstPos.begin(), right.firstPos.end(), std::insert_iterator<std::set<int>>(firstPos, firstPos.begin()));
                else
                    firstPos = left.firstPos;
                if (right.nullable)
                    std::set_union(left.lastPos.begin(), left.lastPos.end(), right.lastPos.begin(), right.lastPos.end(), std::insert_iterator<std::set<int>>(lastPos, lastPos.begin()));
                else
                    lastPos = right.lastPos;
                s.push(Node(firstPos, lastPos, left.nullable && right.nullable));
                for (int idx : left.lastPos)
                {
                    auto it = followPos.find(idx);
                    if (it == followPos.end())
                    {
                        followPos.insert({idx, right.firstPos});
                    }
                    else
                    {
                        it->second.insert(right.firstPos.begin(), right.firstPos.end());
                    }
                }
            }
        }
    }
    followPos.insert({letterIdx - 1, std::set<int>({-1})});
    return {indexToLetter, followPos, s.top()};
}

std::map<char, std::set<int>> groupByLetter(std::map<int, char> &indexToLetter, std::set<int> &positions)
{
    std::map<char, std::set<int>> letterToIndexSubset;
    for (int pos : positions)
    {
        auto it = letterToIndexSubset.find(indexToLetter[pos]);
        if (it == letterToIndexSubset.end())
            letterToIndexSubset.insert({indexToLetter[pos], std::set<int>({pos})});
        else
            it->second.insert(pos);
    }
    return letterToIndexSubset;
}

std::set<int> followPosUnion(std::map<int, std::set<int>> &followPos, std::set<int> &positions)
{
    if (positions.size() == 1)
        return followPos[*positions.begin()];
    std::set<int> un;
    for (int pos : positions)
    {
        auto &posFollowPos = followPos[pos];
        if (posFollowPos.find(-1) != posFollowPos.end())
            continue;
        un.insert(posFollowPos.begin(), posFollowPos.end());
    }
    return un;
}

AFD buildAFD(std::map<int, char> &indexToLetter, std::map<int, std::set<int>> &followPos, Node & root)
{
    AFD afd;
    AFDNode *startState = new AFDNode(root.firstPos);
    afd.startNode = startState;
    std::deque<AFDNode *> q;
    std::set<AFDNode *> visited;
    q.push_back(startState);
    while (!q.empty())
    {
        AFDNode *currentState = q.front();
        q.pop_front();
        if (visited.find(currentState) == visited.end())
        {
            visited.insert(currentState);
            auto letterToIndexSubset = groupByLetter(indexToLetter, currentState->pos);
            for (auto letterToIndexPair : letterToIndexSubset)
            {
                if (letterToIndexPair.first == '#') {
                    afd.finalNodes.insert(currentState);
                    continue;
                }
                std::set<int> nextStatePositions = followPosUnion(followPos, letterToIndexPair.second);
                AFDNode *nextState;
                auto visitedStateIter = std::find_if(visited.begin(), visited.end(), [&nextStatePositions](AFDNode *node)
                                                     { return node->pos == nextStatePositions; });
                auto qStateIter = find_if(q.begin(), q.end(), [&nextStatePositions](AFDNode *node)
                                               { return node->pos == nextStatePositions; });
                if (visitedStateIter != visited.end() || qStateIter != q.end())
                    nextState = visitedStateIter != visited.end() ? *visitedStateIter : *qStateIter;
                else
                {
                    AFDNode *nextState = new AFDNode(nextStatePositions);
                    q.push_back(nextState);
                }
                currentState->nextNode.insert({letterToIndexPair.first, nextState});
            }
        }
    }
    return afd;
}

std::string setToString(const std::set<int> &positions)
{
    std::string s;
    s.reserve(positions.size() * 2 + 3);
    s += "\" ";
    for (int pos : positions)
    {
        s += std::to_string(pos);
        s += ' ';
    }
    s += "\"";
    return s;
}

void writeDOTFile(AFD afd, const std::string &outputFilename)
{
    std::ofstream fout(outputFilename);
    std::queue<AFDNode *> q;
    std::set<AFDNode *> visited;
    fout << "digraph {\n";
    fout << setToString(afd.startNode->pos) << "[color=red]\n";
    for (AFDNode * finalNode : afd.finalNodes) {
        fout << setToString(finalNode->pos) << "[color=blue]\n";
    }
    q.push(afd.startNode);
    while (!q.empty())
    {
        AFDNode *currentNode = q.front();
        q.pop();
        if (visited.find(currentNode) == visited.end())
        {
            visited.insert(currentNode);
            for (auto &transition : currentNode->nextNode)
            {
                fout << setToString(currentNode->pos) << "->" << setToString(transition.second->pos) << "[label=\"" << transition.first << "\"]\n";
                q.push(transition.second);
            }
        }
    }
    fout << "}";
    fout.close();
}
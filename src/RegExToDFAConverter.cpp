#include <algorithm>
#include <exception>
#include "RegExToDFAConverter.h"

void RegExToDFAConverter::parseFile()
{
    std::ifstream fin(filename);
    if (!fin.is_open())
    {
        throw std::runtime_error(filename + " could not be opened");
    }
    readAlphabet(fin);
    readExpression(fin);
    fin.close();
}

void RegExToDFAConverter::readAlphabet(std::ifstream &fin)
{
    std::string alphabet;
    std::getline(fin, alphabet);
    if (!fin)
    {
        throw std::runtime_error("Encountered error while reading alphabet");
    }
    setAlphabet(alphabet);
}

void RegExToDFAConverter::setAlphabet(const std::string &alphabet)
{
    size_t prev_pos = 0;
    size_t pos = alphabet.find(' ');
    while (pos != std::string::npos)
    {
        this->alphabet.insert(alphabet.substr(prev_pos, pos - prev_pos)[0]);
        prev_pos = pos + 1;
        pos = alphabet.find(' ', pos + 1);
    }

    this->alphabet.insert(alphabet.substr(prev_pos)[0]);
}

void RegExToDFAConverter::readExpression(std::ifstream &fin)
{
    std::getline(fin, expression);
    if (!fin)
    {
        throw std::runtime_error("Encountered error while reading expression");
    }
}

void RegExToDFAConverter::buildAFD()
{
    setExtendedPostfixExpression();
    parseExtendedPostfixExpression();
    setAFD();
}

void RegExToDFAConverter::setExtendedPostfixExpression()
{
    extendedPostfixExpression = std::move(convertExpressionToPostfixForm());
    extendedPostfixExpression += "#.";
}

std::string RegExToDFAConverter::convertExpressionToPostfixForm()
{
    std::stack<char> operatorsStack;
    std::queue<char> postfixQueue;

    for (char currentCharacter : expression)
        if (isalpha(currentCharacter))
            postfixQueue.push(currentCharacter);
        else
            handleOperator(currentCharacter, operatorsStack, postfixQueue);

    addRemainingOperators(operatorsStack, postfixQueue);
    return convertPostfixQueueToString(postfixQueue);
}

void RegExToDFAConverter::handleOperator(char op, std::stack<char> &operatorsStack, std::queue<char> &postfixQueue)
{
    if (op == '(' || !operatorsStack.empty() && operatorsStack.top() == '(')
        operatorsStack.push(op);
    else if (op == ')')
    {
        while (operatorsStack.top() != '(')
        {
            postfixQueue.push(operatorsStack.top());
            operatorsStack.pop();
        }
        operatorsStack.pop();
    }
    else
    {
        while (!operatorsStack.empty() && compareOperatorPrecedence(operatorsStack.top(), op))
        {
            postfixQueue.push(operatorsStack.top());
            operatorsStack.pop();
        }
        operatorsStack.push(op);
    }
}

bool RegExToDFAConverter::compareOperatorPrecedence(char op1, char op2)
{
    if (op1 == '*')
        return true;
    if (op1 == '.' && op2 != '*')
        return true;
    if (op1 == '|' && op2 != '*' && op2 != '.')
        return true;
    return false;
}

void RegExToDFAConverter::addRemainingOperators(std::stack<char> &operatorsStack, std::queue<char> &postfixQueue)
{
    while (!operatorsStack.empty())
    {
        postfixQueue.push(operatorsStack.top());
        operatorsStack.pop();
    }
}

std::string RegExToDFAConverter::convertPostfixQueueToString(std::queue<char> &postfixQueue)
{
    std::string result;
    result.reserve(postfixQueue.size() + 2);
    while (!postfixQueue.empty())
    {
        result += postfixQueue.front();
        postfixQueue.pop();
    }
    return result;
}

void RegExToDFAConverter::parseExtendedPostfixExpression()
{
    std::stack<Node> depthFirstStack;
    int letterIndex = 1;
    for (char currentCharacter : extendedPostfixExpression)
    {
        if (isalpha(currentCharacter) || currentCharacter == '#')
        {
            indexToLetter.insert({letterIndex, currentCharacter});
            depthFirstStack.push(Node(std::set<int>({letterIndex}), std::set<int>({letterIndex}), false));
            letterIndex++;
        }
        else
        {
            if (currentCharacter == '*')
                traverseKleeneStarNode(depthFirstStack);
            else if (currentCharacter == '|')
                traverseUnionNode(depthFirstStack);
            else if (currentCharacter == '.')
                traverseConcatenationNode(depthFirstStack);
        }
    }
    followPosTable.insert({letterIndex - 1, std::set<int>({-1})});
    root = depthFirstStack.top();
}

void RegExToDFAConverter::traverseKleeneStarNode(std::stack<Node> & depthFirstStack)
{
    Node top = depthFirstStack.top();
    depthFirstStack.pop();
    depthFirstStack.push(Node(top.firstPos, top.lastPos, true));
    for (int idx : top.lastPos)
    {
        auto it = followPosTable.find(idx);
        if (it == followPosTable.end())
        {
            followPosTable.insert({idx, top.firstPos});
        }
        else
        {
            it->second.insert(top.firstPos.begin(), top.firstPos.end());
        }
    }
}

void RegExToDFAConverter::traverseUnionNode(std::stack<Node> & depthFirstStack)
{
    Node right = depthFirstStack.top();
    depthFirstStack.pop();
    Node left = depthFirstStack.top();
    depthFirstStack.pop();
    std::set<int> firstPos;
    std::set<int> lastPos;
    std::set_union(left.firstPos.begin(), left.firstPos.end(), right.firstPos.begin(), right.firstPos.end(), std::insert_iterator<std::set<int>>(firstPos, firstPos.begin()));
    std::set_union(left.lastPos.begin(), left.lastPos.end(), right.lastPos.begin(), right.lastPos.end(), std::insert_iterator<std::set<int>>(lastPos, lastPos.begin()));
    depthFirstStack.push(Node(firstPos, lastPos, left.nullable || right.nullable));
}

void RegExToDFAConverter::traverseConcatenationNode(std::stack<Node> & depthFirstStack)
{
    Node right = depthFirstStack.top();
    depthFirstStack.pop();
    Node left = depthFirstStack.top();
    depthFirstStack.pop();
    std::set<int> firstPos = std::move(getConcatenationNodeFirstPos(right, left));
    std::set<int> lastPos = std::move(getConcatenationNodeLastPos(right, left));

    depthFirstStack.push(Node(firstPos, lastPos, left.nullable && right.nullable));
    for (int idx : left.lastPos)
    {
        auto it = followPosTable.find(idx);
        if (it == followPosTable.end())
        {
            followPosTable.insert({idx, right.firstPos});
        }
        else
        {
            it->second.insert(right.firstPos.begin(), right.firstPos.end());
        }
    }
}

std::set<int> RegExToDFAConverter::getConcatenationNodeFirstPos(Node &right, Node &left)
{
    std::set<int> firstPos;
    if (left.nullable)
        std::set_union(left.firstPos.begin(), left.firstPos.end(), right.firstPos.begin(), right.firstPos.end(), std::insert_iterator<std::set<int>>(firstPos, firstPos.begin()));
    else
        firstPos = left.firstPos;
    return firstPos;
}

std::set<int> RegExToDFAConverter::getConcatenationNodeLastPos(Node &right, Node &left)
{
    std::set<int> lastPos;
    if (right.nullable)
        std::set_union(left.lastPos.begin(), left.lastPos.end(), right.lastPos.begin(), right.lastPos.end(), std::insert_iterator<std::set<int>>(lastPos, lastPos.begin()));
    else
        lastPos = right.lastPos;
    return lastPos;
}

void RegExToDFAConverter::setAFD()
{
    AFDNode *startState = new AFDNode(root.firstPos);
    afd.startNode = startState;
    std::deque<AFDNode *> breadthFirstQueue;
    std::set<AFDNode *> visitedNodes;
    breadthFirstQueue.push_back(startState);
    while (!breadthFirstQueue.empty())
    {
        AFDNode *currentState = breadthFirstQueue.front();
        breadthFirstQueue.pop_front();
        if (visitedNodes.find(currentState) == visitedNodes.end())
        {
            visitedNodes.insert(currentState);
            addNextStatesForCurrentState(currentState, visitedNodes, breadthFirstQueue);
        }
    }
}

void RegExToDFAConverter::addNextStatesForCurrentState(AFDNode *currentState, std::set<AFDNode *> &visitedNodes, std::deque<AFDNode *> &breadthFirstQueue)
{
    auto stateLetterToIndices = std::move(groupPositionsByLetter(currentState->pos));
    auto stateEqualityPredicate = [](std::set<int> &nextStatePositions)
    { return [&nextStatePositions](AFDNode *node)
      { return node->pos == nextStatePositions; }; };
    for (auto letterToIndicesPair : stateLetterToIndices)
    {
        if (letterToIndicesPair.first == '#')
        {
            afd.finalNodes.insert(currentState);
            continue;
        }
        std::set<int> nextStatePositions = getUnionOfPositionsFollowPos(letterToIndicesPair.second);
        AFDNode *nextState;
        auto visitedStateIter = std::find_if(visitedNodes.begin(), visitedNodes.end(), stateEqualityPredicate(nextStatePositions));
        auto enqueuedStateIter = std::find_if(breadthFirstQueue.begin(), breadthFirstQueue.end(), stateEqualityPredicate(nextStatePositions));

        if (visitedStateIter != visitedNodes.end() || enqueuedStateIter != breadthFirstQueue.end())
            nextState = visitedStateIter != visitedNodes.end() ? *visitedStateIter : *enqueuedStateIter;
        else
        {
            AFDNode *nextState = new AFDNode(nextStatePositions);
            breadthFirstQueue.push_back(nextState);
        }
        currentState->nextNode.insert({letterToIndicesPair.first, nextState});
    }
}

std::map<char, std::set<int>> RegExToDFAConverter::groupPositionsByLetter(std::set<int> &positions)
{
    std::map<char, std::set<int>> groupedPositions;
    for (int pos : positions)
    {
        auto it = groupedPositions.find(indexToLetter[pos]);
        if (it == groupedPositions.end())
            groupedPositions.insert({indexToLetter[pos], std::set<int>({pos})});
        else
            it->second.insert(pos);
    }
    return groupedPositions;
}

std::set<int> RegExToDFAConverter::getUnionOfPositionsFollowPos(std::set<int> &positions)
{
    if (positions.size() == 1)
        return followPosTable[*positions.begin()];
    std::set<int> positionsFollowPosUnion;
    for (int pos : positions)
    {
        auto &followPosOfCurrentPosition = followPosTable[pos];
        positionsFollowPosUnion.insert(followPosOfCurrentPosition.begin(), followPosOfCurrentPosition.end());
    }
    return positionsFollowPosUnion;
}

void RegExToDFAConverter::writeDOTFile(const std::string &outputFilename)
{
    std::ofstream fout(outputFilename);
    std::queue<AFDNode *> breadthFirstQueue;
    std::set<AFDNode *> visited;
    fout << "digraph {\n";
    writeStartAndFinalStates(fout);
    breadthFirstQueue.push(afd.startNode);
    while (!breadthFirstQueue.empty())
    {
        AFDNode *currentNode = breadthFirstQueue.front();
        breadthFirstQueue.pop();
        if (visited.find(currentNode) == visited.end())
        {
            visited.insert(currentNode);
            writeTransitions(fout, currentNode, breadthFirstQueue);
        }
    }
    fout << "}";
    fout.close();
}

void RegExToDFAConverter::writeStartAndFinalStates(std::ofstream & fout) {
    fout << "\"\"[shape=none, height=.0, width=.0]";
    fout << "\"\"->" << std::move(positionsToString(afd.startNode->pos)) << std::endl;
    for (AFDNode *finalNode : afd.finalNodes)
    {
        fout << std::move(positionsToString(finalNode->pos)) << "[peripheries=2]\n";
    }
}

void RegExToDFAConverter::writeTransitions(std::ofstream & fout, AFDNode * currentState, std::queue<AFDNode *>& breadthFirstQueue) {
    for (auto &transition : currentState->nextNode)
    {
        fout << std::move(positionsToString(currentState->pos)) << "->" << std::move(positionsToString(transition.second->pos)) << "[label=\"" << transition.first << "\"]\n";
        breadthFirstQueue.push(transition.second);
    }
}

std::string RegExToDFAConverter::positionsToString(const std::set<int> &positions)
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
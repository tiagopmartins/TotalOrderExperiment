#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <mutex>

/**
 * @brief Interface defining a sequencer.
 * 
 */
class Sequencer {

private:
    int _seqN;    // Sequence number counter

    std::mutex _seqNMutex;

public:
    Sequencer() : _seqN(0) {}

    int seqN() {
        return this->_seqN;
    }

    std::mutex* seqNMutex() {
        return &(this->_seqNMutex);
    }

    void incrementSeqN() {
        this->_seqN++;
    }

    /**
     * @brief Sends a sequence number to a process regarding the
     * message specified by it's message ID.
     * 
     * @param address 
     * @param msgId 
     */
    virtual void sendSequencerNumber(std::string address, int msgId) = 0;

};

#endif
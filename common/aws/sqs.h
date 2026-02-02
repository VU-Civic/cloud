#ifndef __AWS_SQS_HEADER_H__
#define __AWS_SQS_HEADER_H__

#include "aws.h"

class AwsSQS final
{
public:

  // Constructor/destructor
  AwsSQS(const char* __restrict queueURL);
  AwsSQS(const AwsSQS&) = delete;
  AwsSQS& operator=(const AwsSQS&) = delete;
  ~AwsSQS(void);

  // AWS SQS management functionality
  bool sendMessage(const char* __restrict message);
  std::tuple<bool, std::string, std::string> receiveMessage(int secondsToWait = 20);
  bool deleteMessage(const char* __restrict receiptHandle);
  bool purgeQueue(void);

private:

  // Member variables
  std::shared_ptr<Aws::SQS::SQSClient> sqsClient;
  std::string queueURL;
};

#endif  // #ifndef __AWS_SQS_HEADER_H__

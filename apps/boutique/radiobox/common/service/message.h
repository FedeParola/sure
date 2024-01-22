/*
 * Some sort of Copyright
 */

#ifndef __MESSAGE__
#define __MESSAGE__

#include <stdint.h>

/**
 * // -----------------Cart service-----------------
 *
 * service CartService {
 *     rpc AddItem(AddItemRequest) returns (Empty) {}
 *     rpc GetCart(GetCartRequest) returns (Cart) {}
 *     rpc EmptyCart(EmptyCartRequest) returns (Empty) {}
 * }
 *
 * message CartItem {
 *     string product_id = 1;
 *     int32  quantity = 2;
 * }
 *
 * message AddItemRequest {
 *     string user_id = 1;
 *     CartItem item = 2;
 * }
 *
 * message EmptyCartRequest {
 *     string user_id = 1;
 * }
 *
 * message GetCartRequest {
 *     string user_id = 1;
 * }
 *
 * message Cart {
 *     string user_id = 1;
 *     repeated CartItem items = 2;
 * }
 *
 * message Empty {}
 */

typedef struct _cartItem {
	char ProductId[20];
	int32_t Quantity;
} CartItem;

typedef struct _addItemRequest{
	char UserId[50];
	CartItem Item;
} AddItemRequest;

typedef struct _emptyCartRequest{
	char UserId[50];
} EmptyCartRequest;

typedef struct _getCartRequest {
	char UserId[50];
} GetCartRequest;

typedef struct _cart {
	char UserId[50];
	int num_items;
	CartItem Items[10];
} Cart;

typedef struct _getCartRR {
	GetCartRequest req;
	Cart res;
} GetCartRR;

enum cart_command {
	CART_ADD_ITEM,
	CART_GET_CART,
	CART_EMPTY_CART,
};

/**
 * // ---------------Recommendation service----------
 *
 * service RecommendationService {
 *   rpc ListRecommendations(ListRecommendationsRequest) returns (ListRecommendationsResponse){}
 * }
 *
 * message ListRecommendationsRequest {
 *     string user_id = 1;
 *     repeated string product_ids = 2;
 * }
 *
 * message ListRecommendationsResponse {
 *     repeated string product_ids = 1;
 * }
 */

typedef struct _listRecommendationsRequest {
	char user_id[50];
	unsigned num_product_ids;
	char product_ids[10][20];
} ListRecommendationsRequest;

typedef struct _listRecommendationsResponse{
	unsigned num_product_ids;
	char product_ids[10][20];
} ListRecommendationsResponse;

typedef struct _listRecommendationsRR {
	ListRecommendationsRequest req;
	ListRecommendationsResponse res;
} ListRecommendationsRR;

/**
 * // -----------------Currency service-----------------
 *
 * service CurrencyService {
 *     rpc GetSupportedCurrencies(Empty) returns (GetSupportedCurrenciesResponse) {}
 *     rpc Convert(CurrencyConversionRequest) returns (Money) {}
 * }
 *
 * // Represents an amount of money with its currency type.
 * message Money {
 *     // The 3-letter currency code defined in ISO 4217.
 *     string currency_code = 1;
 *
 *     // The whole units of the amount.
 *     // For example if `currencyCode` is `"USD"`, then 1 unit is one US dollar.
 *     int64 units = 2;
 *
 *     // Number of nano (10^-9) units of the amount.
 *     // The value must be between -999,999,999 and +999,999,999 inclusive.
 *     // If `units` is positive, `nanos` must be positive or zero.
 *     // If `units` is zero, `nanos` can be positive, zero, or negative.
 *     // If `units` is negative, `nanos` must be negative or zero.
 *     // For example $-1.75 is represented as `units`=-1 and `nanos`=-750,000,000.
 *     int32 nanos = 3;
 * }
 *
 * message GetSupportedCurrenciesResponse {
 *     // The 3-letter currency code defined in ISO 4217.
 *     repeated string currency_codes = 1;
 * }
 *
 * message CurrencyConversionRequest {
 *     Money from = 1;
 *
 *     // The 3-letter currency code defined in ISO 4217.
 *     string to_code = 2;
 * }
 */

typedef struct _money {
	char CurrencyCode[10];
	int64_t Units;
	int32_t Nanos;
} Money;

typedef struct _getSupportedCurrenciesResponse {
	int num_currencies;
	char CurrencyCodes[6][10];
} GetSupportedCurrenciesResponse;

typedef struct _currencyConversionRequest {
	Money From;
	char ToCode[10];
} CurrencyConversionRequest;

typedef struct _currencyConversionRR {
	CurrencyConversionRequest req;
	Money res;
} CurrencyConversionRR;

enum currency_command {
	CURRENCY_GET_SUPPORTED_CURRENCIES,
	CURRENCY_CONVERT
};

/**
 * // ---------------Product Catalog----------------
 *
 * service ProductCatalogService {
 *     rpc ListProducts(Empty) returns (ListProductsResponse) {}
 *     rpc GetProduct(GetProductRequest) returns (Product) {}
 *     rpc SearchProducts(SearchProductsRequest) returns (SearchProductsResponse) {}
 * }
 *
 * message Product {
 *     string id = 1;
 *     string name = 2;
 *     string description = 3;
 *     string picture = 4;
 *     Money price_usd = 5;
 *
 *     // Categories such as "clothing" or "kitchen" that can be used to look up
 *     // other related products.
 *     repeated string categories = 6;
 * }
 *
 * message ListProductsResponse {
 *     repeated Product products = 1;
 * }
 *
 * message GetProductRequest {
 *     string id = 1;
 * }
 *
 * message SearchProductsRequest {
 *     string query = 1;
 * }
 *
 * message SearchProductsResponse {
 *     repeated Product results = 1;
 * }
 */

typedef struct _product {
	char Id[20];
	char Name[30];
	char Description[100];
	char Picture[60];
	Money PriceUsd;
	int num_categories;
	char Categories[5][20];
} Product;

typedef struct _listProductsResponse {
	int num_products;
	Product Products[9];
} ListProductsResponse;

typedef struct _getProductRequest{
	char Id[20];
} GetProductRequest;

typedef struct _getProductRR{
	GetProductRequest req;
	Product res;
} GetProductRR;

typedef struct _searchProductsRequest {
	char Query[50];
} SearchProductsRequest;

typedef struct _searchProductsResponse {
	int num_products;
	Product Results[9];
} SearchProductsResponse;

typedef struct _searchProductsRR {
	SearchProductsRequest req;
	SearchProductsResponse res;
} SearchProductsRR;

enum productcatalog_command {
	PRODUCTCATALOG_LIST_PRODUCTS,
	PRODUCTCATALOG_GET_PRODUCT,
	PRODUCTCATALOG_SEARCH_PRODUCTS,
};

/**
 * // ---------------Shipping Service----------
 *
 * service ShippingService {
 *     rpc GetQuote(GetQuoteRequest) returns (GetQuoteResponse) {}
 *     rpc ShipOrder(ShipOrderRequest) returns (ShipOrderResponse) {}
 * }
 *
 * message GetQuoteRequest {
 *     Address address = 1;
 *     repeated CartItem items = 2;
 * }
 *
 * message GetQuoteResponse {
 *     Money cost_usd = 1;
 * }
 *
 * message ShipOrderRequest {
 *     Address address = 1;
 *     repeated CartItem items = 2;
 * }
 *
 * message ShipOrderResponse {
 *     string tracking_id = 1;
 * }
 *
 * message Address {
 *     string street_address = 1;
 *     string city = 2;
 *     string state = 3;
 *     string country = 4;
 *     int32 zip_code = 5;
 * }
 */

typedef struct _address {
	char StreetAddress[50];
	char City[15];
	char State[15];
	char Country[15];
	int32_t ZipCode;
} Address;

typedef struct _getQuoteRequest{
	Address address;
	int num_items;
	CartItem Items[10];
} GetQuoteRequest;

typedef struct _getQuoteResponse {
	int conversion_flag;
	Money CostUsd;
} GetQuoteResponse;

typedef struct _getQuoteRR {
	GetQuoteRequest req;
	GetQuoteResponse res;
} GetQuoteRR;

typedef struct _shipOrderRequest {
	Address address;
	CartItem Items[10];
} ShipOrderRequest;

typedef struct _shipOrderResponse{
	char TrackingId[100];
} ShipOrderResponse;

typedef struct _shipOrderRR {
	ShipOrderRequest req;
	ShipOrderResponse res;
} ShipOrderRR;

enum shipping_command {
	SHIPPING_GET_QUOTE,
	SHIPPING_SHIP_ORDER,
};

/**
 * // -------------Payment service-----------------
 *
 * service PaymentService {
 *     rpc Charge(ChargeRequest) returns (ChargeResponse) {}
 * }
 *
 * message CreditCardInfo {
 *     string credit_card_number = 1;
 *     int32 credit_card_cvv = 2;
 *     int32 credit_card_expiration_year = 3;
 *     int32 credit_card_expiration_month = 4;
 * }
 *
 * message ChargeRequest {
 *     Money amount = 1;
 *     CreditCardInfo credit_card = 2;
 * }
 *
 * message ChargeResponse {
 *     string transaction_id = 1;
 * }
 */

typedef struct _creditCardInfo {
	char CreditCardNumber[30];
	int32_t CreditCardCvv;
	int32_t CreditCardExpirationYear;
	int32_t CreditCardExpirationMonth;
} CreditCardInfo;

typedef struct _chargeRequest {
	Money Amount;
	CreditCardInfo CreditCard;
} ChargeRequest;

typedef struct _chargeResponse {
	char TransactionId[40];
} ChargeResponse;

typedef struct _chargeRR {
	ChargeRequest req;
	ChargeResponse res;
} ChargeRR;

/**
 * // -------------Email service-----------------
 *
 * service EmailService {
 *     rpc SendOrderConfirmation(SendOrderConfirmationRequest) returns (Empty) {}
 * }
 *
 * message OrderItem {
 *     CartItem item = 1;
 *     Money cost = 2;
 * }
 *
 * message OrderResult {
 *     string   order_id = 1;
 *     string   shipping_tracking_id = 2;
 *     Money shipping_cost = 3;
 *     Address  shipping_address = 4;
 *     repeated OrderItem items = 5;
 * }
 *
 * message SendOrderConfirmationRequest {
 *     string email = 1;
 *     OrderResult order = 2;
 * }
 */

typedef struct _orderItem {
	CartItem Item;
	Money Cost;
} OrderItem;

typedef struct _orderResult {
	char OrderId[40];
	char ShippingTrackingId[100];
	Money ShippingCost;
	Address ShippingAddress;
	unsigned num_items;
	OrderItem Items[10];
} OrderResult;

typedef struct _sendOrderConfirmationRequest {
	char Email[50];
	OrderResult Order;
} SendOrderConfirmationRequest;

typedef struct _sendOrderConfirmationRR {
	SendOrderConfirmationRequest req;
 } SendOrderConfirmationRR;

/**
 * // -------------Checkout service-----------------
 *
 * service CheckoutService {
 *     rpc PlaceOrder(PlaceOrderRequest) returns (PlaceOrderResponse) {}
 * }
 *
 * message PlaceOrderRequest {
 *     string user_id = 1;
 *     string user_currency = 2;
 *
 *     Address address = 3;
 *     string email = 5;
 *     CreditCardInfo credit_card = 6;
 * }
 *
 * message PlaceOrderResponse {
 *     OrderResult order = 1;
 * }
 */

typedef struct _placeOrderRequest{
	char UserId[50];
	char UserCurrency[5];
	Address address;
	char Email[50];
	CreditCardInfo CreditCard;
} PlaceOrderRequest;

typedef struct _placeOrderResponse {
	OrderResult order;
} PlaceOrderResponse;

typedef struct _placeorderRR {
	PlaceOrderRequest req;
	PlaceOrderResponse res;
} PlaceOrderRR;

/**
 * // ------------Ad service------------------
 *
 * service AdService {
 *     rpc GetAds(AdRequest) returns (AdResponse) {}
 * }
 *
 * message AdRequest {
 *     // List of important key words from the current page describing the context.
 *     repeated string context_keys = 1;
 * }
 *
 * message AdResponse {
 *     repeated Ad ads = 1;
 * }
 *
 * message Ad {
 *     // url to redirect to when an ad is clicked.
 *     string redirect_url = 1;
 *
 *     // short advertisement text to display.
 *     string text = 2;
 * }
 */

typedef struct _ad {
	char RedirectUrl[100];
	char Text[100];
} Ad;

typedef struct _adrequest {
	int num_context_keys;
	char ContextKeys[10][100];
} AdRequest;

typedef struct _adresponse {
	int num_ads;
	Ad Ads[10];
} AdResponse;

typedef struct _adrr {
	AdRequest req;
	AdResponse res;
} AdRR;

struct rpc {
	/* Unique id used to identify the RPC by the caller */
	unsigned id;
	/* Command of the RPC, see enum <service>_command */
	int command;
	/* Body of the RPC */
	char rr[0];
};

#endif /* __MESSAGE__ */

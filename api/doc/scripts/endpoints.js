const generateGETListEndpoint = (data) => {
        return {
                method: "GET",
                name: `${data.name.plural} List`,
                queryParameters: [
                        { name: "q", type: "string" },
                        {
                                name: "sort",
                                type: "select",
                                options: [
                                        {
                                                name: null,
                                                label: "No sorting",
                                        },
                                        {
                                                name: "asc",
                                                label: "Ascending",
                                        },
                                        {
                                                name: "desc",
                                                label: "Descending",
                                        },
                                ],
                        },
                ],
                responses: [
                        {
                                code: 200,
                                response: [data.schema],
                        },
                ],
                uri: data.uri,
        };
};

const generateGETSingleEndpoint = (data) => {
        return {
                method: "GET",
                name: `${data.name.singular}`,
                responses: [
                        {
                                code: 200,
                                response:
                                        data.response?.["get-single"] ??
                                        data.schema,
                        },
                ],
                uri: `${data.uri}/{${data.primaryKey.name.toUpperCase()}}`,
                uriParameters: [
                        {
                                defaultValue: data.primaryKey.defaultValue,
                                name: `${data.name.singular} ${data.primaryKey.name}`,
                                parameter: `{${data.primaryKey.name.toUpperCase()}}`,
                                type: data.primaryKey.type,
                        },
                ],
        };
};

const generatePOSTEndpoint = (data) => {
        return {
                body: data.body?.post?.schema ?? data.schema,
                defaultBody: data.body?.post?.defaultValue,
                method: "POST",
                name: `Add ${data.name.singular}`,
                responses: [
                        {
                                code: 201,
                                response: `${data.name.singular} has been successfully created`,
                        },
                ],
                uri: data.uri,
        };
};

const generatePUTEndpoint = (data) => {
        return {
                body: data.body?.put?.schema ?? data.schema,
                defaultBody: data.body?.put.defaultValue,
                method: "PUT",
                name: `Edit ${data.name.singular}`,
                responses: [
                        {
                                code: 200,
                                response: `${data.name.singular} has been successfully edited`,
                        },
                ],
                uri: `${data.uri}/{${data.primaryKey.name.toUpperCase()}}`,
                uriParameters: [
                        {
                                defaultValue: data.primaryKey.defaultValue,
                                name: `${data.name.singular} ${data.primaryKey.name}`,
                                parameter: `{${data.primaryKey.name.toUpperCase()}}`,
                                type: data.primaryKey.type,
                        },
                ],
        };
};

const generateDELETEEndpoint = (data) => {
        return {
                method: "DELETE",
                name: `Delete ${data.name.singular}`,
                responses: [
                        {
                                code: 200,
                                response: `${data.name.singular} has been successfully deleted`,
                        },
                ],
                uri: `${data.uri}/{${data.primaryKey.name.toUpperCase()}}`,
                uriParameters: [
                        {
                                defaultValue: data.primaryKey.defaultValue,
                                name: `${data.name.singular} ${data.primaryKey.name}`,
                                parameter: `{${data.primaryKey.name.toUpperCase()}}`,
                                type: data.primaryKey.type,
                        },
                ],
        };
};

const endpointGenerators = {
        "GET-list": generateGETListEndpoint,
        "GET-single": generateGETSingleEndpoint,
        POST: generatePOSTEndpoint,
        PUT: generatePUTEndpoint,
        DELETE: generateDELETEEndpoint,
};

const generateTablesEndpoints = (data) => {
        const tables = [];

        data.forEach((el) => {
                const table = {
                        name: el.name.plural,
                        endpoints: [],
                };

                let includedEndpoints = Object.values(endpointGenerators);

                if (
                        el.includedEndpoints !== undefined &&
                        el.includedEndpoints.length > 0
                ) {
                        includedEndpoints = [];

                        console.log(el.includedEndpoints);
                        el.includedEndpoints.forEach((i) => {
                                includedEndpoints.push(endpointGenerators[i]);
                        });
                }

                includedEndpoints.forEach((fct) => {
                        table.endpoints.push(fct(el));
                });

                console.log(table);

                tables.push(table);
        });

        return tables;
};
